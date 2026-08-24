#include "AnnotationDocument.h"

#include <QFont>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>
#include <utility>

namespace snipnexs {

void AnnotationDocument::begin(AnnotationTool tool, const QPointF& point)
{
    if (tool == AnnotationTool::None || tool == AnnotationTool::Text) {
        return;
    }

    Annotation annotation;
    annotation.tool = tool;
    annotation.points = {point};
    if (tool != AnnotationTool::Pen) {
        annotation.points.append(point);
    }
    draft_ = std::move(annotation);
}

void AnnotationDocument::addText(const QString& text, const QPointF& point)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    Annotation annotation;
    annotation.tool = AnnotationTool::Text;
    annotation.points = {point};
    annotation.text = trimmed;
    items_.resize(appliedCount_);
    items_.append(std::move(annotation));
    appliedCount_ = items_.size();
    draft_.reset();
}

void AnnotationDocument::update(const QPointF& point)
{
    if (!draft_.has_value()) {
        return;
    }

    if (draft_->tool == AnnotationTool::Pen) {
        if (QLineF(draft_->points.constLast(), point).length() >= 1.0) {
            draft_->points.append(point);
        }
        return;
    }
    draft_->points[1] = point;
}

void AnnotationDocument::commit()
{
    if (!draft_.has_value()) {
        return;
    }

    const bool hasLength = draft_->points.size() > 1
        && QLineF(draft_->points.constFirst(), draft_->points.constLast()).length() >= 2.0;
    const bool validPen = draft_->tool == AnnotationTool::Pen && !draft_->points.isEmpty();
    if (!validPen && !hasLength) {
        draft_.reset();
        return;
    }

    items_.resize(appliedCount_);
    items_.append(std::move(*draft_));
    appliedCount_ = items_.size();
    draft_.reset();
}

void AnnotationDocument::cancel()
{
    draft_.reset();
}

void AnnotationDocument::clear()
{
    items_.clear();
    appliedCount_ = 0;
    draft_.reset();
}

bool AnnotationDocument::undo()
{
    if (!canUndo()) {
        return false;
    }
    --appliedCount_;
    draft_.reset();
    return true;
}

bool AnnotationDocument::redo()
{
    if (!canRedo()) {
        return false;
    }
    ++appliedCount_;
    draft_.reset();
    return true;
}

bool AnnotationDocument::canUndo() const
{
    return appliedCount_ > 0;
}

bool AnnotationDocument::canRedo() const
{
    return appliedCount_ < items_.size();
}

int AnnotationDocument::itemCount() const
{
    return static_cast<int>(appliedCount_);
}

void AnnotationDocument::paint(QPainter& painter) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    for (qsizetype i = 0; i < appliedCount_; ++i) {
        paintAnnotation(painter, items_.at(i));
    }
    if (draft_.has_value()) {
        paintAnnotation(painter, *draft_);
    }
    painter.restore();
}

void AnnotationDocument::paintAnnotation(QPainter& painter, const Annotation& annotation)
{
    if (annotation.points.isEmpty()) {
        return;
    }

    if (annotation.tool == AnnotationTool::Text) {
        QFont font = painter.font();
        font.setPixelSize(annotation.fontPixelSize);
        font.setWeight(QFont::DemiBold);
        painter.setFont(font);
        painter.setPen(annotation.color);
        painter.drawText(annotation.points.constFirst(), annotation.text);
        return;
    }

    painter.setPen(QPen(
        annotation.color,
        annotation.width,
        Qt::SolidLine,
        Qt::RoundCap,
        Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    if (annotation.tool == AnnotationTool::Pen) {
        if (annotation.points.size() == 1) {
            painter.drawPoint(annotation.points.constFirst());
            return;
        }
        QPainterPath path(annotation.points.constFirst());
        for (qsizetype i = 1; i < annotation.points.size(); ++i) {
            path.lineTo(annotation.points.at(i));
        }
        painter.drawPath(path);
        return;
    }

    if (annotation.points.size() < 2) {
        return;
    }
    const QPointF start = annotation.points.at(0);
    const QPointF end = annotation.points.at(1);

    if (annotation.tool == AnnotationTool::Rectangle) {
        painter.drawRect(QRectF(start, end).normalized());
        return;
    }

    if (annotation.tool == AnnotationTool::Arrow) {
        painter.drawLine(start, end);
        const QLineF line(end, start);
        if (line.length() < 1.0) {
            return;
        }

        const qreal arrowLength = std::max<qreal>(10.0, annotation.width * 4.0);
        const qreal angle = std::atan2(line.dy(), line.dx());
        const QPointF left = end + QPointF(
            std::cos(angle + 0.5) * arrowLength,
            std::sin(angle + 0.5) * arrowLength);
        const QPointF right = end + QPointF(
            std::cos(angle - 0.5) * arrowLength,
            std::sin(angle - 0.5) * arrowLength);
        painter.drawLine(end, left);
        painter.drawLine(end, right);
    }
}

} // namespace snipnexs
