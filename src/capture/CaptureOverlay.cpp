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
#include <array>
#include <utility>

namespace snipnexs {

namespace {

constexpr int kHandleSize = 8;
constexpr int kHandleHitPadding = 3;
constexpr int kMinimumSelectionSize = 3;

}

CaptureOverlay::CaptureOverlay(QPixmap screenshot, QWidget* parent)
    : CaptureOverlay(std::move(screenshot), {}, parent)
{
}

CaptureOverlay::CaptureOverlay(
    QPixmap screenshot, QList<QImage> history, QWidget* parent)
    : QWidget(parent)
    , liveScreenshot_(std::move(screenshot))
    , screenshot_(liveScreenshot_)
    , dimmedScreenshot_(screenshot_)
    , history_(std::move(history))
    , historyIndex_(history_.size())
    , devicePixelRatio_(screenshot_.devicePixelRatio())
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("SnipNexs Capture"));
    setAttribute(Qt::WA_DeleteOnClose);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    rebuildDimmedScreenshot();

    toolbar_ = new QFrame(this);
    toolbar_->setObjectName(QStringLiteral("captureToolbar"));
    auto* layout = new QHBoxLayout(toolbar_);
    layout->setContentsMargins(9, 7, 9, 7);
    layout->setSpacing(7);

    sizeLabel_ = new QLabel(toolbar_);
    auto* penButton = new QPushButton(tr("画笔"), toolbar_);
    auto* rectangleButton = new QPushButton(tr("矩形"), toolbar_);
    auto* arrowButton = new QPushButton(tr("箭头"), toolbar_);
    undoButton_ = new QPushButton(tr("撤销"), toolbar_);
    redoButton_ = new QPushButton(tr("重做"), toolbar_);
    auto* ocrButton = new QPushButton(tr("识字"), toolbar_);
    ocrButton->setObjectName(QStringLiteral("ocrButton"));
    auto* pinButton = new QPushButton(tr("贴图"), toolbar_);
    recordButton_ = new QPushButton(tr("录屏"), toolbar_);
    recordButton_->setObjectName(QStringLiteral("recordButton"));
    auto* copyButton = new QPushButton(tr("复制"), toolbar_);
    auto* saveButton = new QPushButton(tr("保存"), toolbar_);
    auto* cancelButton = new QPushButton(tr("取消"), toolbar_);
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
    layout->addWidget(ocrButton);
    layout->addWidget(pinButton);
    layout->addWidget(recordButton_);
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
    connect(ocrButton, &QPushButton::clicked, this, &CaptureOverlay::acceptOcr);
    connect(pinButton, &QPushButton::clicked, this, &CaptureOverlay::acceptPin);
    connect(recordButton_, &QPushButton::clicked, this, &CaptureOverlay::acceptRecord);
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
    interaction_ = Interaction::None;
    activeResizeHandle_ = ResizeHandle::None;
    if (selection_.width() >= kMinimumSelectionSize
        && selection_.height() >= kMinimumSelectionSize) {
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
        update();
        return;
    }
    if (auto* button = toolButtons_->button(static_cast<int>(tool))) {
        button->setChecked(true);
    }
    setCursor(Qt::CrossCursor);
    update();
}

void CaptureOverlay::keyPressEvent(QKeyEvent* event)
{
    if (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_Comma) {
        switchHistory(-1);
        return;
    }
    if (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_Period) {
        switchHistory(1);
        return;
    }
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
    const QPoint point = event->position().toPoint();
    if (interaction_ != Interaction::None) {
        updateInteraction(point);
        return;
    }
    updateCursorForPosition(point);
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

    const QPoint point = event->position().toPoint();
    if (selectionGeometryEditable()) {
        activeResizeHandle_ = resizeHandleAt(point);
        if (activeResizeHandle_ != ResizeHandle::None) {
            interaction_ = Interaction::Resize;
        } else if (selection_.contains(point)) {
            interaction_ = Interaction::Move;
        }
    }

    toolbar_->hide();
    interactionOrigin_ = point;
    interactionStartSelection_ = selection_;
    if (interaction_ == Interaction::None) {
        annotations_.clear();
        updateEditorActions();
        interaction_ = Interaction::Create;
        selection_ = QRect(interactionOrigin_, QSize(1, 1));
    }
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
    if (interaction_ == Interaction::None || event->button() != Qt::LeftButton) {
        return;
    }

    const QPoint point = event->position().toPoint();
    updateInteraction(point);
    interaction_ = Interaction::None;
    activeResizeHandle_ = ResizeHandle::None;
    setSelection(selection_);
    updateCursorForPosition(point);
}

void CaptureOverlay::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawPixmap(rect(), dimmedScreenshot_);

    if (!selection_.isValid()) {
        const QString hint = tr("拖动鼠标选择区域 · Esc 或右键取消");
        const QRect hintRect = fontMetrics().boundingRect(hint).adjusted(-14, -9, 14, 9);
        QRect centered = hintRect;
        centered.moveCenter(QPoint(width() / 2, 42));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(15, 20, 26, 210));
        painter.drawRoundedRect(centered, 6, 6);
        painter.setPen(QColor(235, 240, 245));
        painter.drawText(centered, Qt::AlignCenter, hint);
        drawHistoryHint(painter);
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

    if (selectionGeometryEditable()) {
        static constexpr std::array handles {
            ResizeHandle::TopLeft,
            ResizeHandle::Top,
            ResizeHandle::TopRight,
            ResizeHandle::Right,
            ResizeHandle::BottomRight,
            ResizeHandle::Bottom,
            ResizeHandle::BottomLeft,
            ResizeHandle::Left,
        };
        painter.setPen(QPen(QColor(57, 208, 190), 1));
        painter.setBrush(QColor(245, 249, 252));
        for (const ResizeHandle handle : handles) {
            painter.drawRect(handleRect(handle));
        }
    }
    drawHistoryHint(painter);
}

void CaptureOverlay::activateHistoryIndex(qsizetype index)
{
    if (index < 0 || index > history_.size()) {
        return;
    }

    historyIndex_ = index;
    annotations_.clear();
    updateEditorActions();
    setAnnotationTool(AnnotationTool::None);

    if (historyIndex_ == history_.size()) {
        activeHistoryImage_ = {};
        screenshot_ = liveScreenshot_;
        devicePixelRatio_ = screenshot_.devicePixelRatio();
        recordButton_->setEnabled(true);
        selection_ = {};
        toolbar_->hide();
        rebuildDimmedScreenshot();
        update();
        return;
    }

    activeHistoryImage_ = history_.at(historyIndex_);
    screenshot_ = liveScreenshot_;
    devicePixelRatio_ = screenshot_.devicePixelRatio();
    recordButton_->setEnabled(false);
    const QImage& image = activeHistoryImage_;
    QSize targetSize = image.deviceIndependentSize().toSize();
    const QSize maximumSize(
        qMax(1, qRound(width() * 0.9)),
        qMax(1, qRound(height() * 0.8)));
    if (targetSize.width() > maximumSize.width()
        || targetSize.height() > maximumSize.height()) {
        targetSize.scale(maximumSize, Qt::KeepAspectRatio);
    }

    QRect targetRect(QPoint(), targetSize);
    targetRect.moveCenter(rect().center());
    {
        QPainter painter(&screenshot_);
        painter.drawImage(targetRect, image);
    }
    rebuildDimmedScreenshot();
    setSelection(targetRect);
}

void CaptureOverlay::drawHistoryHint(QPainter& painter) const
{
    if (history_.isEmpty()) {
        return;
    }

    const QString hint = historyIndex_ == history_.size()
        ? tr("当前屏幕 · 按 , 查看截图记录")
        : tr("截图记录 %1/%2 · 按 , / . 切换")
              .arg(historyIndex_ + 1)
              .arg(history_.size());
    QRect hintRect = fontMetrics().boundingRect(hint).adjusted(-12, -7, 12, 7);
    hintRect.moveTopRight(QPoint(width() - 16, 16));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(15, 20, 26, 210));
    painter.drawRoundedRect(hintRect, 6, 6);
    painter.setPen(QColor(235, 240, 245));
    painter.drawText(hintRect, Qt::AlignCenter, hint);
}

void CaptureOverlay::rebuildDimmedScreenshot()
{
    dimmedScreenshot_ = screenshot_;
    QPainter dimmer(&dimmedScreenshot_);
    dimmer.fillRect(
        QRectF(QPointF(0, 0), dimmedScreenshot_.deviceIndependentSize()),
        QColor(0, 0, 0, 115));
}

void CaptureOverlay::switchHistory(int offset)
{
    if (history_.isEmpty()) {
        return;
    }
    activateHistoryIndex(std::clamp<qsizetype>(
        historyIndex_ + offset, 0, history_.size()));
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

    if (!activeHistoryImage_.isNull()) {
        QImage image = activeHistoryImage_.copy();
        if (annotations_.itemCount() == 0) {
            return image;
        }

        const QSizeF logicalSize = image.deviceIndependentSize();
        QPainter painter(&image);
        painter.scale(
            logicalSize.width() / selection_.width(),
            logicalSize.height() / selection_.height());
        painter.translate(-selection_.left(), -selection_.top());
        annotations_.paint(painter);
        return image;
    }

    const QRect pixelRect = logicalToPixelRect(
        selection_, devicePixelRatio_, screenshot_.size());
    if (!pixelRect.isValid()) {
        return {};
    }

    QImage image = screenshot_.toImage().copy(pixelRect);
    image.setDevicePixelRatio(devicePixelRatio_);
    {
        QPainter painter(&image);
        painter.translate(-selection_.topLeft());
        annotations_.paint(painter);
    }
    return image;
}

QRect CaptureOverlay::selectedPixelRect() const
{
    if (!activeHistoryImage_.isNull()) {
        return {};
    }
    return logicalToPixelRect(selection_, devicePixelRatio_, screenshot_.size());
}

void CaptureOverlay::acceptCopy()
{
    const QImage image = selectedImage();
    if (!image.isNull()) {
        emit copyRequested(image);
    }
}

void CaptureOverlay::acceptOcr()
{
    const QImage image = selectedImage();
    if (!image.isNull()) {
        emit ocrRequested(image);
    }
}

void CaptureOverlay::acceptPin()
{
    const QImage image = selectedImage();
    if (!image.isNull()) {
        emit pinRequested(image);
    }
}

void CaptureOverlay::acceptRecord()
{
    const QRect pixelRect = selectedPixelRect();
    if (pixelRect.isValid()) {
        emit recordRequested(pixelRect);
    }
}

void CaptureOverlay::acceptSave()
{
    const QImage image = selectedImage();
    if (!image.isNull()) {
        emit saveRequested(image);
    }
}

QRect CaptureOverlay::handleRect(ResizeHandle handle) const
{
    QPoint center;
    switch (handle) {
    case ResizeHandle::TopLeft:
        center = selection_.topLeft();
        break;
    case ResizeHandle::Top:
        center = QPoint(selection_.center().x(), selection_.top());
        break;
    case ResizeHandle::TopRight:
        center = selection_.topRight();
        break;
    case ResizeHandle::Right:
        center = QPoint(selection_.right(), selection_.center().y());
        break;
    case ResizeHandle::BottomRight:
        center = selection_.bottomRight();
        break;
    case ResizeHandle::Bottom:
        center = QPoint(selection_.center().x(), selection_.bottom());
        break;
    case ResizeHandle::BottomLeft:
        center = selection_.bottomLeft();
        break;
    case ResizeHandle::Left:
        center = QPoint(selection_.left(), selection_.center().y());
        break;
    case ResizeHandle::None:
        return {};
    }

    const int offset = kHandleSize / 2;
    return QRect(center.x() - offset, center.y() - offset, kHandleSize, kHandleSize);
}

CaptureOverlay::ResizeHandle CaptureOverlay::resizeHandleAt(const QPoint& point) const
{
    static constexpr std::array handles {
        ResizeHandle::TopLeft,
        ResizeHandle::Top,
        ResizeHandle::TopRight,
        ResizeHandle::Right,
        ResizeHandle::BottomRight,
        ResizeHandle::Bottom,
        ResizeHandle::BottomLeft,
        ResizeHandle::Left,
    };
    for (const ResizeHandle handle : handles) {
        if (handleRect(handle).adjusted(
                -kHandleHitPadding,
                -kHandleHitPadding,
                kHandleHitPadding,
                kHandleHitPadding)
                .contains(point)) {
            return handle;
        }
    }
    return ResizeHandle::None;
}

bool CaptureOverlay::selectionGeometryEditable() const
{
    return selection_.isValid()
        && activeHistoryImage_.isNull()
        && annotationTool_ == AnnotationTool::None
        && annotations_.itemCount() == 0
        && !annotations_.canRedo();
}

void CaptureOverlay::positionToolbar()
{
    const QSize pixelSize = activeHistoryImage_.isNull()
        ? logicalToPixelRect(selection_, devicePixelRatio_, screenshot_.size()).size()
        : activeHistoryImage_.size();
    sizeLabel_->setText(QStringLiteral("%1 × %2")
        .arg(pixelSize.width())
        .arg(pixelSize.height()));
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

void CaptureOverlay::updateCursorForPosition(const QPoint& point)
{
    if (!selectionGeometryEditable()) {
        setCursor(Qt::CrossCursor);
        return;
    }

    switch (resizeHandleAt(point)) {
    case ResizeHandle::TopLeft:
    case ResizeHandle::BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        return;
    case ResizeHandle::TopRight:
    case ResizeHandle::BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        return;
    case ResizeHandle::Top:
    case ResizeHandle::Bottom:
        setCursor(Qt::SizeVerCursor);
        return;
    case ResizeHandle::Left:
    case ResizeHandle::Right:
        setCursor(Qt::SizeHorCursor);
        return;
    case ResizeHandle::None:
        setCursor(selection_.contains(point) ? Qt::SizeAllCursor : Qt::CrossCursor);
        return;
    }
}

void CaptureOverlay::updateInteraction(const QPoint& point)
{
    const QPoint bounded(
        std::clamp(point.x(), 0, width() - 1),
        std::clamp(point.y(), 0, height() - 1));

    if (interaction_ == Interaction::Create) {
        selection_ = QRect(interactionOrigin_, bounded).normalized();
    } else if (interaction_ == Interaction::Move) {
        selection_ = interactionStartSelection_.translated(bounded - interactionOrigin_);
        if (selection_.left() < 0) {
            selection_.moveLeft(0);
        }
        if (selection_.top() < 0) {
            selection_.moveTop(0);
        }
        if (selection_.right() >= width()) {
            selection_.moveRight(width() - 1);
        }
        if (selection_.bottom() >= height()) {
            selection_.moveBottom(height() - 1);
        }
    } else if (interaction_ == Interaction::Resize) {
        selection_ = interactionStartSelection_;
        switch (activeResizeHandle_) {
        case ResizeHandle::TopLeft:
        case ResizeHandle::Left:
        case ResizeHandle::BottomLeft:
            selection_.setLeft(std::min(
                bounded.x(), interactionStartSelection_.right() - kMinimumSelectionSize + 1));
            break;
        default:
            break;
        }
        switch (activeResizeHandle_) {
        case ResizeHandle::TopRight:
        case ResizeHandle::Right:
        case ResizeHandle::BottomRight:
            selection_.setRight(std::max(
                bounded.x(), interactionStartSelection_.left() + kMinimumSelectionSize - 1));
            break;
        default:
            break;
        }
        switch (activeResizeHandle_) {
        case ResizeHandle::TopLeft:
        case ResizeHandle::Top:
        case ResizeHandle::TopRight:
            selection_.setTop(std::min(
                bounded.y(), interactionStartSelection_.bottom() - kMinimumSelectionSize + 1));
            break;
        default:
            break;
        }
        switch (activeResizeHandle_) {
        case ResizeHandle::BottomLeft:
        case ResizeHandle::Bottom:
        case ResizeHandle::BottomRight:
            selection_.setBottom(std::max(
                bounded.y(), interactionStartSelection_.top() + kMinimumSelectionSize - 1));
            break;
        default:
            break;
        }
    }
    update();
}

} // namespace snipnexs
