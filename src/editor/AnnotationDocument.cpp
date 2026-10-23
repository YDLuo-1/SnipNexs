#include "AnnotationDocument.h"

#include <QFont>
#include <QImage>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>

#include <algorithm>
#include <cmath>
#include <utility>

namespace snipnexs {

namespace {

// Effect brushes are wide by design: they redact content, they do not draw.
constexpr qreal kEffectStrokeWidth = 32.0;
constexpr qreal kMosaicBlockLogical = 10.0;
constexpr qreal kBlurRadiusLogical = 6.0;
constexpr int kBlurIterations = 2;

bool isEffectTool(AnnotationTool tool)
{
    return tool == AnnotationTool::Mosaic || tool == AnnotationTool::Blur;
}

bool isStrokeTool(AnnotationTool tool)
{
    return tool == AnnotationTool::Pen || isEffectTool(tool);
}

QPainterPath effectRegion(const Annotation& annotation)
{
    if (annotation.points.isEmpty()) {
        return {};
    }
    if (annotation.points.size() == 1) {
        // A single click is a dot, not an empty stroked path.
        QPainterPath dot;
        const qreal radius = annotation.width / 2.0;
        dot.addEllipse(annotation.points.constFirst(), radius, radius);
        return dot;
    }
    QPainterPath center(annotation.points.constFirst());
    for (qsizetype i = 1; i < annotation.points.size(); ++i) {
        center.lineTo(annotation.points.at(i));
    }
    QPainterPathStroker stroker;
    stroker.setWidth(annotation.width);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    return stroker.createStroke(center);
}

QImage pixelated(const QImage& patch, int block)
{
    if (block <= 1 || patch.width() <= 0 || patch.height() <= 0) {
        return patch;
    }
    // Two nearest-neighbor passes produce crisp square blocks.
    return patch
        .scaled(qMax(1, patch.width() / block),
            qMax(1, patch.height() / block),
            Qt::IgnoreAspectRatio, Qt::FastTransformation)
        .scaled(patch.width(), patch.height(),
            Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

void blurLine(const QRgb* source, QRgb* destination, int count, int radius)
{
    const int window = 2 * radius + 1;
    int sums[4] = {0, 0, 0, 0};
    const auto clamped = [&](int index) {
        return source[std::clamp(index, 0, count - 1)];
    };
    for (int i = -radius; i <= radius; ++i) {
        const QRgb value = clamped(i);
        sums[0] += qRed(value);
        sums[1] += qGreen(value);
        sums[2] += qBlue(value);
        sums[3] += qAlpha(value);
    }
    for (int i = 0; i < count; ++i) {
        destination[i] = qRgba(sums[0] / window, sums[1] / window,
            sums[2] / window, sums[3] / window);
        const QRgb leaving = clamped(i - radius);
        sums[0] -= qRed(leaving);
        sums[1] -= qGreen(leaving);
        sums[2] -= qBlue(leaving);
        sums[3] -= qAlpha(leaving);
        const QRgb entering = clamped(i + radius + 1);
        sums[0] += qRed(entering);
        sums[1] += qGreen(entering);
        sums[2] += qBlue(entering);
        sums[3] += qAlpha(entering);
    }
}

// Separable box blur with clamped edges; premultiplied channel averaging is
// valid per channel. Two iterations approximate a gaussian at low cost.
void boxBlurInPlace(QImage& image, int radius, int iterations)
{
    if (radius < 1 || image.width() < 1 || image.height() < 1) {
        return;
    }
    if (image.format() != QImage::Format_ARGB32_Premultiplied) {
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
    std::vector<QRgb> line(image.width());
    std::vector<QRgb> column(image.height());
    std::vector<QRgb> blurred(qMax(image.width(), image.height()));
    for (int iteration = 0; iteration < iterations; ++iteration) {
        for (int y = 0; y < image.height(); ++y) {
            QRgb* row = reinterpret_cast<QRgb*>(image.scanLine(y));
            blurLine(row, line.data(), image.width(), radius);
            std::copy(line.begin(), line.begin() + image.width(), row);
        }
        for (int x = 0; x < image.width(); ++x) {
            for (int y = 0; y < image.height(); ++y) {
                column[y] = reinterpret_cast<const QRgb*>(
                    image.constScanLine(y))[x];
            }
            blurLine(column.data(), blurred.data(), image.height(), radius);
            for (int y = 0; y < image.height(); ++y) {
                reinterpret_cast<QRgb*>(image.scanLine(y))[x] = blurred[y];
            }
        }
    }
}

void paintEffect(
    QPainter& painter, const Annotation& annotation, const AnnotationSource& source)
{
    const QPainterPath region = effectRegion(annotation);
    if (region.isEmpty()) {
        return;
    }
    if (source.image == nullptr) {
        // Without pixels the redaction intent still has to be visible.
        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(120, 130, 140, 110));
        painter.drawPath(region);
        painter.restore();
        return;
    }

    const QRectF logicalRect = region.boundingRect();
    QRect physicalRect = source.toImagePixels
                             .mapRect(logicalRect)
                             .toAlignedRect()
                             .intersected(source.image->rect());
    if (physicalRect.isEmpty()) {
        return;
    }
    QImage patch = source.image->copy(physicalRect);
    const qreal scale = source.toImagePixels.m11();
    if (annotation.tool == AnnotationTool::Mosaic) {
        patch = pixelated(patch, qMax(4, qRound(kMosaicBlockLogical * scale)));
    } else {
        boxBlurInPlace(
            patch, qMax(2, qRound(kBlurRadiusLogical * scale)), kBlurIterations);
    }

    painter.save();
    painter.setClipPath(region);
    painter.drawImage(logicalRect, patch,
        QRectF(0, 0, physicalRect.width(), physicalRect.height()));
    painter.restore();
}

} // namespace

void AnnotationDocument::begin(AnnotationTool tool, const QPointF& point)
{
    if (tool == AnnotationTool::None || tool == AnnotationTool::Text) {
        return;
    }

    Annotation annotation;
    annotation.tool = tool;
    annotation.points = {point};
    if (isEffectTool(tool)) {
        annotation.width = kEffectStrokeWidth;
    }
    if (tool != AnnotationTool::Pen && !isEffectTool(tool)) {
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

    if (isStrokeTool(draft_->tool)) {
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
    const bool validStroke = isStrokeTool(draft_->tool)
        && !draft_->points.isEmpty();
    if (!validStroke && !hasLength) {
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

void AnnotationDocument::paint(
    QPainter& painter, const AnnotationSource& source) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    for (qsizetype i = 0; i < appliedCount_; ++i) {
        paintAnnotation(painter, items_.at(i), source);
    }
    if (draft_.has_value()) {
        paintAnnotation(painter, *draft_, source);
    }
    painter.restore();
}

void AnnotationDocument::paintAnnotation(
    QPainter& painter, const Annotation& annotation, const AnnotationSource& source)
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

    if (isEffectTool(annotation.tool)) {
        paintEffect(painter, annotation, source);
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
