#include "CaptureOverlay.h"

#include "CaptureGeometry.h"

#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QShowEvent>

#include <algorithm>
#include <utility>

namespace snipnexs {

CaptureOverlay::CaptureOverlay(QPixmap screenshot, QWidget* parent)
    : QWidget(parent)
    , screenshot_(std::move(screenshot))
    , dimmedScreenshot_(screenshot_)
    , devicePixelRatio_(screenshot_.devicePixelRatio())
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setWindowTitle(QStringLiteral("SnipNexs Capture"));
    setAttribute(Qt::WA_DeleteOnClose);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);

    QPainter dimmer(&dimmedScreenshot_);
    dimmer.fillRect(
        QRectF(QPointF(0, 0), dimmedScreenshot_.deviceIndependentSize()),
        QColor(0, 0, 0, 115));

    toolbar_ = new QFrame(this);
    toolbar_->setObjectName(QStringLiteral("captureToolbar"));
    auto* layout = new QHBoxLayout(toolbar_);
    layout->setContentsMargins(9, 7, 9, 7);
    layout->setSpacing(7);

    sizeLabel_ = new QLabel(toolbar_);
    auto* penButton = new QPushButton(QStringLiteral("画笔"), toolbar_);
    auto* rectangleButton = new QPushButton(QStringLiteral("矩形"), toolbar_);
    auto* arrowButton = new QPushButton(QStringLiteral("箭头"), toolbar_);
    undoButton_ = new QPushButton(QStringLiteral("撤销"), toolbar_);
    redoButton_ = new QPushButton(QStringLiteral("重做"), toolbar_);
    auto* pinButton = new QPushButton(QStringLiteral("贴图"), toolbar_);
    auto* copyButton = new QPushButton(QStringLiteral("复制"), toolbar_);
    auto* saveButton = new QPushButton(QStringLiteral("保存"), toolbar_);
    auto* cancelButton = new QPushButton(QStringLiteral("取消"), toolbar_);
    copyButton->setObjectName(QStringLiteral("accentButton"));
    toolButtons_ = new QButtonGroup(this);
    toolButtons_->setExclusive(true);
    toolButtons_->addButton(penButton, static_cast<int>(AnnotationTool::Pen));
    toolButtons_->addButton(rectangleButton, static_cast<int>(AnnotationTool::Rectangle));
    toolButtons_->addButton(arrowButton, static_cast<int>(AnnotationTool::Arrow));
    for (auto* button : toolButtons_->buttons()) {
        button->setCheckable(true);
    }
    layout->addWidget(sizeLabel_);
    layout->addWidget(penButton);
    layout->addWidget(rectangleButton);
    layout->addWidget(arrowButton);
    layout->addWidget(undoButton_);
    layout->addWidget(redoButton_);
    layout->addWidget(pinButton);
    layout->addWidget(copyButton);
    layout->addWidget(saveButton);
    layout->addWidget(cancelButton);
    toolbar_->hide();

    toolbar_->setStyleSheet(QStringLiteral(R"(
        QFrame#captureToolbar { background: #151b22; border: 1px solid #34414e; border-radius: 7px; }
        QLabel { color: #b9c5d0; padding: 0 5px; }
        QPushButton { min-height: 28px; padding: 0 12px; color: #eaf0f5; background: #27323e; border: 0; border-radius: 5px; }
        QPushButton:hover { background: #344351; }
        QPushButton:checked { color: #092824; background: #f0ba45; font-weight: 600; }
        QPushButton:disabled { color: #667380; background: #202832; }
        QPushButton#accentButton { color: #092824; background: #39d0be; font-weight: 600; }
        QPushButton#accentButton:hover { background: #52dfce; }
    )"));

    connect(copyButton, &QPushButton::clicked, this, &CaptureOverlay::acceptCopy);
    connect(pinButton, &QPushButton::clicked, this, &CaptureOverlay::acceptPin);
    connect(saveButton, &QPushButton::clicked, this, &CaptureOverlay::acceptSave);
    connect(cancelButton, &QPushButton::clicked, this, &CaptureOverlay::canceled);
    connect(toolButtons_, &QButtonGroup::idClicked, this, [this](int id) {
        setAnnotationTool(static_cast<AnnotationTool>(id));
    });
    connect(undoButton_, &QPushButton::clicked, this, [this]() {
        const bool changed = annotations_.undo();
        Q_UNUSED(changed);
        updateEditorActions();
        update();
    });
    connect(redoButton_, &QPushButton::clicked, this, [this]() {
        const bool changed = annotations_.redo();
        Q_UNUSED(changed);
        updateEditorActions();
        update();
    });
    updateEditorActions();
}

void CaptureOverlay::setSelection(const QRect& selection)
{
    selection_ = selection.normalized().intersected(rect());
    dragging_ = false;
    if (selection_.width() >= 3 && selection_.height() >= 3) {
        positionToolbar();
        toolbar_->show();
    } else {
        selection_ = {};
        toolbar_->hide();
    }
    update();
}

void CaptureOverlay::setAnnotationTool(AnnotationTool tool)
{
    annotationTool_ = tool;
    if (tool == AnnotationTool::None) {
        toolButtons_->setExclusive(false);
        for (auto* button : toolButtons_->buttons()) {
            button->setChecked(false);
        }
        toolButtons_->setExclusive(true);
        setCursor(Qt::CrossCursor);
        return;
    }
    if (auto* button = toolButtons_->button(static_cast<int>(tool))) {
        button->setChecked(true);
    }
    setCursor(Qt::CrossCursor);
}

void CaptureOverlay::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Undo)) {
        const bool changed = annotations_.undo();
        Q_UNUSED(changed);
        updateEditorActions();
        update();
        return;
    }
    if (event->matches(QKeySequence::Redo)) {
        const bool changed = annotations_.redo();
        Q_UNUSED(changed);
        updateEditorActions();
        update();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (annotationDrawing_) {
            annotations_.cancel();
            annotationDrawing_ = false;
            toolbar_->show();
            update();
            return;
        }
        if (annotationTool_ != AnnotationTool::None) {
            setAnnotationTool(AnnotationTool::None);
            return;
        }
        emit canceled();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        acceptCopy();
        return;
    }
    QWidget::keyPressEvent(event);
}

void CaptureOverlay::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && selection_.contains(event->position().toPoint())) {
        acceptCopy();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void CaptureOverlay::mouseMoveEvent(QMouseEvent* event)
{
    if (annotationDrawing_) {
        annotations_.update(event->position());
        update();
        return;
    }
    if (dragging_) {
        updateSelection(event->position().toPoint());
    }
}

void CaptureOverlay::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        emit canceled();
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (annotationTool_ != AnnotationTool::None) {
        if (selection_.contains(event->position().toPoint())) {
            annotationDrawing_ = true;
            toolbar_->hide();
            annotations_.begin(annotationTool_, event->position());
            update();
        }
        return;
    }

    toolbar_->hide();
    annotations_.clear();
    updateEditorActions();
    dragging_ = true;
    dragOrigin_ = event->position().toPoint();
    selection_ = QRect(dragOrigin_, QSize(1, 1));
    update();
}

void CaptureOverlay::mouseReleaseEvent(QMouseEvent* event)
{
    if (annotationDrawing_ && event->button() == Qt::LeftButton) {
        const QPoint point = event->position().toPoint();
        const QPoint bounded(
            std::clamp(point.x(), selection_.left(), selection_.right()),
            std::clamp(point.y(), selection_.top(), selection_.bottom()));
        annotations_.update(bounded);
        annotations_.commit();
        annotationDrawing_ = false;
        updateEditorActions();
        positionToolbar();
        toolbar_->show();
        update();
        return;
    }
    if (!dragging_ || event->button() != Qt::LeftButton) {
        return;
    }

    updateSelection(event->position().toPoint());
    dragging_ = false;
    setSelection(selection_);
}

void CaptureOverlay::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawPixmap(rect(), dimmedScreenshot_);

    if (!selection_.isValid()) {
        const QString hint = QStringLiteral("拖动鼠标选择区域 · Esc 或右键取消");
        const QRect hintRect = fontMetrics().boundingRect(hint).adjusted(-14, -9, 14, 9);
        QRect centered = hintRect;
        centered.moveCenter(QPoint(width() / 2, 42));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(15, 20, 26, 210));
        painter.drawRoundedRect(centered, 6, 6);
        painter.setPen(QColor(235, 240, 245));
        painter.drawText(centered, Qt::AlignCenter, hint);
        return;
    }

    painter.save();
    painter.setClipRect(selection_);
    painter.drawPixmap(rect(), screenshot_);
    annotations_.paint(painter);
    painter.restore();

    painter.setPen(QPen(QColor(57, 208, 190), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(selection_.adjusted(0, 0, -1, -1));
}

void CaptureOverlay::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    raise();
    activateWindow();
    setFocus(Qt::ActiveWindowFocusReason);
}

QImage CaptureOverlay::selectedImage() const
{
    if (!selection_.isValid()) {
        return {};
    }

    const QRect pixelRect = logicalToPixelRect(
        selection_, devicePixelRatio_, screenshot_.size());
    if (!pixelRect.isValid()) {
        return {};
    }

    QImage image = screenshot_.toImage().copy(pixelRect);
    image.setDevicePixelRatio(1.0);
    {
        QPainter painter(&image);
        painter.scale(devicePixelRatio_, devicePixelRatio_);
        painter.translate(-selection_.topLeft());
        annotations_.paint(painter);
    }
    return image;
}

void CaptureOverlay::acceptCopy()
{
    const QImage image = selectedImage();
    if (!image.isNull()) {
        emit copyRequested(image);
    }
}

void CaptureOverlay::acceptPin()
{
    const QImage image = selectedImage();
    if (!image.isNull()) {
        emit pinRequested(image);
    }
}

void CaptureOverlay::acceptSave()
{
    const QImage image = selectedImage();
    if (!image.isNull()) {
        emit saveRequested(image);
    }
}

void CaptureOverlay::positionToolbar()
{
    const QRect pixelRect = logicalToPixelRect(
        selection_, devicePixelRatio_, screenshot_.size());
    sizeLabel_->setText(
        QStringLiteral("%1 × %2").arg(pixelRect.width()).arg(pixelRect.height()));
    toolbar_->adjustSize();

    const int maximumX = std::max(8, width() - toolbar_->width() - 8);
    const int x = std::clamp(selection_.right() - toolbar_->width() + 1, 8, maximumX);
    int y = selection_.bottom() + 9;
    if (y + toolbar_->height() > height() - 8) {
        y = selection_.top() - toolbar_->height() - 9;
    }
    y = std::clamp(y, 8, std::max(8, height() - toolbar_->height() - 8));
    toolbar_->move(x, y);
}

void CaptureOverlay::updateEditorActions()
{
    undoButton_->setEnabled(annotations_.canUndo());
    redoButton_->setEnabled(annotations_.canRedo());
}

void CaptureOverlay::updateSelection(const QPoint& point)
{
    const QPoint bounded(
        std::clamp(point.x(), 0, width() - 1),
        std::clamp(point.y(), 0, height() - 1));
    selection_ = QRect(dragOrigin_, bounded).normalized();
    update();
}

} // namespace snipnexs
