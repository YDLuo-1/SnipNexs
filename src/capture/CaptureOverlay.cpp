#include "CaptureOverlay.h"

#include "CaptureGeometry.h"
#include "ToolbarIcons.h"

#include <QApplication>
#include <QButtonGroup>
#include <QClipboard>
#include <QCursor>
#include <QEvent>
#include <QFrame>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSignalBlocker>

#include <algorithm>
#include <array>
#include <utility>

namespace snipnexs {

namespace {

constexpr int kHandleSize = 8;
constexpr int kHandleHitPadding = 3;
constexpr int kMinimumSelectionSize = 3;

void configureToolbarButton(
    QPushButton* button,
    ToolbarIcon icon,
    const QString& tooltip)
{
    button->setIcon(makeToolbarIcon(icon));
    button->setIconSize(QSize(23, 23));
    button->setToolTip(tooltip);
    button->setAccessibleName(tooltip);
    button->setFixedSize(36, 34);
}

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

    captureHint_ = new QLabel(
        tr("拖动自定义区域 · 单击自动选择窗口 · Esc 或右键取消"), this);
    captureHint_->setObjectName(QStringLiteral("captureHint"));
    captureHint_->setAttribute(Qt::WA_TransparentForMouseEvents);
    QFont captureHintFont = captureHint_->font();
    captureHintFont.setPixelSize(16);
    captureHintFont.setWeight(QFont::Medium);
    captureHint_->setFont(captureHintFont);
    captureHint_->setStyleSheet(QStringLiteral(
        "color: #ebf0f5; background: rgba(15, 20, 26, 210); "
        "border: 1px solid #34414e; border-radius: 6px; padding: 8px 13px;"));
    captureHint_->adjustSize();

    toolbar_ = new QFrame(this);
    toolbar_->setObjectName(QStringLiteral("captureToolbar"));
    toolbar_->setCursor(Qt::ArrowCursor);
    auto* layout = new QHBoxLayout(toolbar_);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(2);

    auto* penButton = new QPushButton(toolbar_);
    auto* rectangleButton = new QPushButton(toolbar_);
    auto* arrowButton = new QPushButton(toolbar_);
    auto* textButton = new QPushButton(toolbar_);
    textButton->setObjectName(QStringLiteral("textButton"));
    colorPickerButton_ = new QPushButton(toolbar_);
    colorPickerButton_->setObjectName(QStringLiteral("colorPickerButton"));
    undoButton_ = new QPushButton(toolbar_);
    redoButton_ = new QPushButton(toolbar_);
    auto* ocrButton = new QPushButton(toolbar_);
    ocrButton->setObjectName(QStringLiteral("ocrButton"));
    auto* pinButton = new QPushButton(toolbar_);
    pinButton->setObjectName(QStringLiteral("pinButton"));
    recordButton_ = new QPushButton(toolbar_);
    recordButton_->setObjectName(QStringLiteral("recordButton"));
    auto* copyButton = new QPushButton(toolbar_);
    auto* saveButton = new QPushButton(toolbar_);
    saveButton->setObjectName(QStringLiteral("saveButton"));
    auto* cancelButton = new QPushButton(toolbar_);
    copyButton->setObjectName(QStringLiteral("copyButton"));
    configureToolbarButton(penButton, ToolbarIcon::Pen, tr("画笔"));
    configureToolbarButton(rectangleButton, ToolbarIcon::Rectangle, tr("矩形"));
    configureToolbarButton(arrowButton, ToolbarIcon::Arrow, tr("箭头"));
    configureToolbarButton(textButton, ToolbarIcon::Text, tr("文字"));
    configureToolbarButton(colorPickerButton_, ToolbarIcon::ColorPicker, tr("取色"));
    colorPickerButton_->setCheckable(true);
    configureToolbarButton(undoButton_, ToolbarIcon::Undo, tr("撤销"));
    configureToolbarButton(redoButton_, ToolbarIcon::Redo, tr("重做"));
    configureToolbarButton(ocrButton, ToolbarIcon::Ocr, tr("识字"));
    configureToolbarButton(pinButton, ToolbarIcon::Pin, tr("贴图"));
    configureToolbarButton(recordButton_, ToolbarIcon::Record, tr("录屏"));
    configureToolbarButton(copyButton, ToolbarIcon::Copy, tr("复制"));
    configureToolbarButton(saveButton, ToolbarIcon::Save, tr("保存"));
    configureToolbarButton(cancelButton, ToolbarIcon::Cancel, tr("取消"));
    toolButtons_ = new QButtonGroup(this);
    toolButtons_->setExclusive(true);
    toolButtons_->addButton(penButton, static_cast<int>(AnnotationTool::Pen));
    toolButtons_->addButton(rectangleButton, static_cast<int>(AnnotationTool::Rectangle));
    toolButtons_->addButton(arrowButton, static_cast<int>(AnnotationTool::Arrow));
    toolButtons_->addButton(textButton, static_cast<int>(AnnotationTool::Text));
    for (auto* button : toolButtons_->buttons()) {
        button->setCheckable(true);
    }
    layout->addWidget(penButton);
    layout->addWidget(rectangleButton);
    layout->addWidget(arrowButton);
    layout->addWidget(textButton);
    layout->addWidget(colorPickerButton_);
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
        QFrame#captureToolbar { background: #f7f9fb; border: 1px solid #b8c3cd; border-radius: 5px; }
        QPushButton { padding: 0; background: transparent; border: 0; border-radius: 4px; }
        QPushButton:hover { background: #e6edf3; }
        QPushButton:pressed { background: #d8e2e9; }
        QPushButton:checked { background: #238bda; }
        QPushButton:disabled { background: transparent; }
    )"));

    selectionSizeLabel_ = new QLabel(this);
    selectionSizeLabel_->setObjectName(QStringLiteral("selectionSizeLabel"));
    selectionSizeLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
    QFont sizeFont = selectionSizeLabel_->font();
    sizeFont.setPixelSize(15);
    sizeFont.setWeight(QFont::DemiBold);
    selectionSizeLabel_->setFont(sizeFont);
    selectionSizeLabel_->setStyleSheet(QStringLiteral(
        "color: #ffffff; background: rgba(24, 31, 38, 220); "
        "border-radius: 4px; padding: 5px 9px;"));
    selectionSizeLabel_->hide();

    textEditor_ = new QLineEdit(this);
    textEditor_->setObjectName(QStringLiteral("textEditor"));
    textEditor_->setPlaceholderText(tr("输入文字，回车完成"));
    textEditor_->setMinimumWidth(220);
    textEditor_->setFixedHeight(38);
    textEditor_->setStyleSheet(QStringLiteral(
        "QLineEdit { color: #1a242d; background: #ffffff; "
        "border: 2px solid #238bda; border-radius: 4px; padding: 4px 8px; "
        "font-size: 16px; selection-background-color: #238bda; }"));
    textEditor_->installEventFilter(this);
    textEditor_->hide();

    connect(copyButton, &QPushButton::clicked, this, &CaptureOverlay::acceptCopy);
    connect(ocrButton, &QPushButton::clicked, this, &CaptureOverlay::acceptOcr);
    connect(pinButton, &QPushButton::clicked, this, &CaptureOverlay::acceptPin);
    connect(recordButton_, &QPushButton::clicked, this, &CaptureOverlay::acceptRecord);
    connect(saveButton, &QPushButton::clicked, this, &CaptureOverlay::acceptSave);
    connect(cancelButton, &QPushButton::clicked, this, &CaptureOverlay::canceled);
    connect(colorPickerButton_, &QPushButton::clicked, this, [this](bool checked) {
        setColorPickerActive(checked);
    });
    connect(textEditor_, &QLineEdit::returnPressed, this, &CaptureOverlay::commitTextEditing);
    connect(textEditor_, &QLineEdit::editingFinished, this, &CaptureOverlay::commitTextEditing);
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
    hoveredWindowTarget_ = {};
    pressedWindowTarget_ = {};
    selection_ = selection.normalized().intersected(rect());
    interaction_ = Interaction::None;
    activeResizeHandle_ = ResizeHandle::None;
    if (selection_.width() >= kMinimumSelectionSize
        && selection_.height() >= kMinimumSelectionSize) {
        positionSelectionSizeLabel();
        positionToolbar();
        toolbar_->show();
    } else {
        selection_ = {};
        toolbar_->hide();
        positionSelectionSizeLabel();
    }
    update();
}

void CaptureOverlay::setWindowTargets(QList<QRect> targets)
{
    windowTargets_ = std::move(targets);
    updateWindowTarget(mapFromGlobal(QCursor::pos()));
}

void CaptureOverlay::setAnnotationTool(AnnotationTool tool)
{
    if (textEditor_->isVisible() && tool != AnnotationTool::Text) {
        commitTextEditing();
    }
    if (colorPickerActive_) {
        colorPickerActive_ = false;
        const QSignalBlocker blocker(colorPickerButton_);
        colorPickerButton_->setChecked(false);
    }
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

bool CaptureOverlay::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == textEditor_ && event->type() == QEvent::KeyPress) {
        const auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            cancelTextEditing();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void CaptureOverlay::keyPressEvent(QKeyEvent* event)
{
    if (colorPickerActive_) {
        if (event->key() == Qt::Key_Escape) {
            setColorPickerActive(false);
            return;
        }
        if (event->key() == Qt::Key_Shift) {
            colorPickerHex_ = !colorPickerHex_;
            update();
            return;
        }
        if (event->key() == Qt::Key_C) {
            copyPickedColor();
            return;
        }
    }
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
    if (event->button() == Qt::LeftButton
        && annotationTool_ == AnnotationTool::None
        && !colorPickerActive_
        && selection_.contains(event->position().toPoint())) {
        acceptCopy();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void CaptureOverlay::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint point = event->position().toPoint();
    updateCaptureHintVisibility(point);
    if (colorPickerActive_) {
        if (selection_.contains(point)) {
            colorPickerPoint_ = point;
            update();
        }
        setCursor(Qt::CrossCursor);
        return;
    }
    if (annotationDrawing_) {
        annotations_.update(event->position());
        update();
        return;
    }
    if (interaction_ != Interaction::None) {
        updateInteraction(point);
        return;
    }
    updateWindowTarget(point);
    updateCursorForPosition(point);
}

void CaptureOverlay::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        if (colorPickerActive_) {
            setColorPickerActive(false);
            return;
        }
        emit canceled();
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }

    const QPoint point = event->position().toPoint();
    updateCaptureHintVisibility(point);
    if (colorPickerActive_) {
        if (selection_.contains(point)) {
            colorPickerPoint_ = point;
            copyPickedColor();
            setColorPickerActive(false);
        }
        return;
    }
    if (annotationTool_ == AnnotationTool::Text) {
        if (selection_.contains(point)) {
            startTextEditing(point);
        }
        return;
    }
    if (annotationTool_ != AnnotationTool::None) {
        if (selection_.contains(point)) {
            annotationDrawing_ = true;
            toolbar_->hide();
            annotations_.begin(annotationTool_, event->position());
            update();
        }
        return;
    }

    if (selectionGeometryEditable()) {
        activeResizeHandle_ = resizeHandleAt(point);
        if (activeResizeHandle_ != ResizeHandle::None) {
            interaction_ = Interaction::Resize;
        } else if (selection_.contains(point)) {
            interaction_ = Interaction::Move;
        }
    }

    pressedWindowTarget_ = selection_.isValid() ? QRect() : windowTargetAt(point);
    hoveredWindowTarget_ = {};

    toolbar_->hide();
    interactionOrigin_ = point;
    interactionStartSelection_ = selection_;
    if (interaction_ == Interaction::None) {
        annotations_.clear();
        updateEditorActions();
        interaction_ = Interaction::Create;
        selection_ = QRect(interactionOrigin_, QSize(1, 1));
    }
    positionSelectionSizeLabel();
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
    const bool useWindowTarget = interaction_ == Interaction::Create
        && pressedWindowTarget_.isValid()
        && (point - interactionOrigin_).manhattanLength()
            < QApplication::startDragDistance();
    if (useWindowTarget) {
        selection_ = pressedWindowTarget_;
    } else {
        updateInteraction(point);
    }
    interaction_ = Interaction::None;
    activeResizeHandle_ = ResizeHandle::None;
    pressedWindowTarget_ = {};
    setSelection(selection_);
    updateCursorForPosition(point);
}

void CaptureOverlay::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawPixmap(rect(), dimmedScreenshot_);

    const QRect previewRect = selection_.isValid() ? selection_ : hoveredWindowTarget_;
    if (previewRect.isValid()) {
        painter.save();
        painter.setClipRect(previewRect);
        painter.drawPixmap(rect(), screenshot_);
        if (selection_.isValid()) {
            annotations_.paint(painter);
        }
        painter.restore();

        painter.setPen(QPen(QColor(57, 208, 190), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(previewRect.adjusted(0, 0, -1, -1));
    }

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
    drawColorPicker(painter);
}

void CaptureOverlay::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    positionCaptureHint();
    positionSelectionSizeLabel();
    if (toolbar_->isVisible()) {
        positionToolbar();
    }
}

void CaptureOverlay::activateHistoryIndex(qsizetype index)
{
    if (index < 0 || index > history_.size()) {
        return;
    }

    historyIndex_ = index;
    cancelTextEditing();
    setColorPickerActive(false);
    annotations_.clear();
    updateEditorActions();
    setAnnotationTool(AnnotationTool::None);

    if (historyIndex_ == history_.size()) {
        activeHistoryImage_ = {};
        screenshot_ = liveScreenshot_;
        devicePixelRatio_ = screenshot_.devicePixelRatio();
        recordButton_->setEnabled(true);
        selection_ = {};
        hoveredWindowTarget_ = {};
        toolbar_->hide();
        positionSelectionSizeLabel();
        rebuildDimmedScreenshot();
        updateWindowTarget(mapFromGlobal(QCursor::pos()));
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
    painter.save();
    QFont hintFont = painter.font();
    hintFont.setPixelSize(16);
    hintFont.setWeight(QFont::Medium);
    painter.setFont(hintFont);
    QRect hintRect = QFontMetrics(hintFont).boundingRect(hint).adjusted(-13, -8, 13, 8);
    hintRect.moveTopRight(QPoint(width() - 16, 16));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(15, 20, 26, 210));
    painter.drawRoundedRect(hintRect, 6, 6);
    painter.setPen(QColor(235, 240, 245));
    painter.drawText(hintRect, Qt::AlignCenter, hint);
    painter.restore();
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

void CaptureOverlay::setColorPickerActive(bool active)
{
    if (active && !selection_.isValid()) {
        const QSignalBlocker blocker(colorPickerButton_);
        colorPickerButton_->setChecked(false);
        return;
    }
    if (colorPickerActive_ == active) {
        return;
    }

    if (active) {
        commitTextEditing();
        setAnnotationTool(AnnotationTool::None);
        colorPickerActive_ = true;
        const QPoint cursor = mapFromGlobal(QCursor::pos());
        colorPickerPoint_ = selection_.contains(cursor) ? cursor : selection_.center();
        toolbar_->hide();
    } else {
        colorPickerActive_ = false;
        if (selection_.isValid()) {
            positionToolbar();
            toolbar_->show();
        }
    }
    {
        const QSignalBlocker blocker(colorPickerButton_);
        colorPickerButton_->setChecked(colorPickerActive_);
    }
    updateCursorForPosition(colorPickerPoint_);
    update();
}

void CaptureOverlay::startTextEditing(const QPoint& point)
{
    if (!selection_.isValid() || !selection_.contains(point)) {
        return;
    }
    commitTextEditing();
    textAnchor_ = point;
    const int editorWidth = std::min(280, std::max(160, width() - point.x() - 8));
    int editorX = point.x();
    if (editorX + editorWidth > width() - 8) {
        editorX = std::max(8, width() - editorWidth - 8);
    }
    int editorY = point.y();
    if (editorY + textEditor_->height() > height() - 8) {
        editorY = std::max(8, point.y() - textEditor_->height());
    }
    textEditor_->setFixedWidth(editorWidth);
    textEditor_->move(editorX, editorY);
    textEditor_->clear();
    toolbar_->hide();
    textEditor_->show();
    textEditor_->raise();
    textEditor_->setFocus(Qt::MouseFocusReason);
}

void CaptureOverlay::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    positionCaptureHint();
    positionSelectionSizeLabel();
    raise();
    activateWindow();
    setFocus(Qt::ActiveWindowFocusReason);
    const QPoint cursorPosition = mapFromGlobal(QCursor::pos());
    updateCaptureHintVisibility(cursorPosition);
    updateWindowTarget(cursorPosition);
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
    commitTextEditing();
    const QImage image = selectedImage();
    if (!image.isNull()) {
        emit copyRequested(image);
    }
}

void CaptureOverlay::acceptOcr()
{
    commitTextEditing();
    const QImage image = selectedImage();
    if (!image.isNull()) {
        emit ocrRequested(image);
    }
}

void CaptureOverlay::acceptPin()
{
    commitTextEditing();
    const QImage image = selectedImage();
    if (!image.isNull()) {
        emit pinRequested(image, mapToGlobal(selection_.topLeft()));
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
    commitTextEditing();
    const QImage image = selectedImage();
    if (!image.isNull()) {
        emit saveRequested(image);
    }
}

void CaptureOverlay::cancelTextEditing()
{
    if (textEditor_ == nullptr || !textEditor_->isVisible()) {
        return;
    }
    const QSignalBlocker blocker(textEditor_);
    textEditor_->hide();
    textEditor_->clear();
    if (selection_.isValid()) {
        positionToolbar();
        toolbar_->show();
    }
    setFocus(Qt::OtherFocusReason);
    update();
}

QColor CaptureOverlay::colorAt(const QPoint& point, QPoint* sourcePoint) const
{
    if (!selection_.isValid() || !selection_.contains(point)) {
        return {};
    }

    QImage source;
    QPoint pixel;
    if (activeHistoryImage_.isNull()) {
        source = screenshot_.toImage();
        pixel = QPoint(
            qFloor(point.x() * devicePixelRatio_),
            qFloor(point.y() * devicePixelRatio_));
    } else {
        source = activeHistoryImage_;
        const qreal relativeX = qreal(point.x() - selection_.left())
            / std::max(1, selection_.width());
        const qreal relativeY = qreal(point.y() - selection_.top())
            / std::max(1, selection_.height());
        pixel = QPoint(
            qFloor(relativeX * source.width()),
            qFloor(relativeY * source.height()));
    }
    if (source.isNull()) {
        return {};
    }

    pixel.setX(std::clamp(pixel.x(), 0, source.width() - 1));
    pixel.setY(std::clamp(pixel.y(), 0, source.height() - 1));
    if (sourcePoint != nullptr) {
        *sourcePoint = pixel;
    }
    return source.pixelColor(pixel);
}

void CaptureOverlay::commitTextEditing()
{
    if (textEditor_ == nullptr || !textEditor_->isVisible()) {
        return;
    }

    const QString text = textEditor_->text();
    const QPoint baseline(
        textAnchor_.x(),
        std::min(selection_.bottom(), textAnchor_.y() + 22));
    {
        const QSignalBlocker blocker(textEditor_);
        textEditor_->hide();
        textEditor_->clear();
    }
    annotations_.addText(text, baseline);
    updateEditorActions();
    if (selection_.isValid()) {
        positionToolbar();
        toolbar_->show();
    }
    setFocus(Qt::OtherFocusReason);
    update();
}

void CaptureOverlay::copyPickedColor()
{
    const QColor color = colorAt(colorPickerPoint_);
    if (!color.isValid()) {
        return;
    }
    const QString value = colorPickerHex_
        ? color.name(QColor::HexRgb).toUpper()
        : QStringLiteral("%1, %2, %3")
              .arg(color.red())
              .arg(color.green())
              .arg(color.blue());
    QGuiApplication::clipboard()->setText(value);
}

void CaptureOverlay::drawColorPicker(QPainter& painter) const
{
    if (!colorPickerActive_) {
        return;
    }

    QPoint sourcePoint;
    const QColor selectedColor = colorAt(colorPickerPoint_, &sourcePoint);
    if (!selectedColor.isValid()) {
        return;
    }
    const QImage source = activeHistoryImage_.isNull()
        ? screenshot_.toImage()
        : activeHistoryImage_;

    constexpr int cellSize = 9;
    constexpr int sampleCount = 9;
    constexpr int gridSize = cellSize * sampleCount;
    const QSize panelSize(250, 126);
    QPoint panelPosition = colorPickerPoint_ + QPoint(18, 18);
    if (panelPosition.x() + panelSize.width() > width() - 8) {
        panelPosition.setX(colorPickerPoint_.x() - panelSize.width() - 18);
    }
    if (panelPosition.y() + panelSize.height() > height() - 8) {
        panelPosition.setY(colorPickerPoint_.y() - panelSize.height() - 18);
    }
    panelPosition.setX(std::clamp(panelPosition.x(), 8, std::max(8, width() - panelSize.width() - 8)));
    panelPosition.setY(std::clamp(panelPosition.y(), 8, std::max(8, height() - panelSize.height() - 8)));
    const QRect panelRect(panelPosition, panelSize);
    const QPoint gridOrigin = panelPosition + QPoint(10, 10);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(QColor(90, 102, 113), 1));
    painter.setBrush(QColor(20, 26, 32, 238));
    painter.drawRoundedRect(panelRect, 6, 6);

    for (int y = 0; y < sampleCount; ++y) {
        for (int x = 0; x < sampleCount; ++x) {
            const QPoint sample(
                std::clamp(sourcePoint.x() + x - sampleCount / 2, 0, source.width() - 1),
                std::clamp(sourcePoint.y() + y - sampleCount / 2, 0, source.height() - 1));
            painter.fillRect(
                QRect(gridOrigin + QPoint(x * cellSize, y * cellSize), QSize(cellSize, cellSize)),
                source.pixelColor(sample));
        }
    }
    const QRect centerCell(
        gridOrigin + QPoint((sampleCount / 2) * cellSize, (sampleCount / 2) * cellSize),
        QSize(cellSize, cellSize));
    painter.setPen(QPen(Qt::white, 2));
    painter.drawRect(centerCell.adjusted(0, 0, -1, -1));
    painter.setPen(QPen(QColor(35, 139, 218), 1));
    painter.drawRect(centerCell.adjusted(-2, -2, 1, 1));

    const int textLeft = gridOrigin.x() + gridSize + 12;
    QFont valueFont = painter.font();
    valueFont.setPixelSize(14);
    valueFont.setWeight(QFont::DemiBold);
    painter.setFont(valueFont);
    painter.setPen(Qt::white);
    painter.drawText(
        QRect(textLeft, panelPosition.y() + 12, 135, 24),
        Qt::AlignLeft | Qt::AlignVCenter,
        QStringLiteral("(%1, %2)").arg(sourcePoint.x()).arg(sourcePoint.y()));
    painter.fillRect(
        QRect(textLeft, panelPosition.y() + 43, 20, 20),
        selectedColor);
    painter.setPen(QPen(QColor(220, 228, 235), 1));
    painter.drawRect(QRect(textLeft, panelPosition.y() + 43, 20, 20));
    painter.setPen(Qt::white);
    const QString value = colorPickerHex_
        ? selectedColor.name(QColor::HexRgb).toUpper()
        : QStringLiteral("%1, %2, %3")
              .arg(selectedColor.red())
              .arg(selectedColor.green())
              .arg(selectedColor.blue());
    painter.drawText(
        QRect(textLeft + 28, panelPosition.y() + 40, 105, 26),
        Qt::AlignLeft | Qt::AlignVCenter,
        value);

    QFont hintFont = painter.font();
    hintFont.setPixelSize(12);
    hintFont.setWeight(QFont::Normal);
    painter.setFont(hintFont);
    painter.setPen(QColor(203, 213, 222));
    painter.drawText(
        QRect(panelPosition.x() + 10, panelPosition.y() + 96, panelSize.width() - 20, 20),
        Qt::AlignCenter,
        tr("按 C 复制 · Shift 切换 RGB/HEX"));
    painter.restore();
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
        && !colorPickerActive_
        && annotations_.itemCount() == 0
        && !annotations_.canRedo();
}

void CaptureOverlay::positionCaptureHint()
{
    captureHint_->adjustSize();
    captureHint_->move(16, std::max(16, height() - captureHint_->height() - 16));
}

void CaptureOverlay::positionSelectionSizeLabel()
{
    const QRect target = selection_.isValid() ? selection_ : hoveredWindowTarget_;
    if (!target.isValid()) {
        selectionSizeLabel_->hide();
        return;
    }

    const QSize pixelSize = activeHistoryImage_.isNull()
        ? logicalToPixelRect(target, devicePixelRatio_, screenshot_.size()).size()
        : activeHistoryImage_.size();
    if (!pixelSize.isValid()) {
        selectionSizeLabel_->hide();
        return;
    }
    selectionSizeLabel_->setText(QStringLiteral("%1 × %2 px")
        .arg(pixelSize.width())
        .arg(pixelSize.height()));
    selectionSizeLabel_->adjustSize();

    int x = target.left();
    int y = target.top() - selectionSizeLabel_->height() - 7;
    if (y < 8) {
        y = target.top() + 7;
    }
    x = std::clamp(x, 8, std::max(8, width() - selectionSizeLabel_->width() - 8));
    y = std::clamp(y, 8, std::max(8, height() - selectionSizeLabel_->height() - 8));
    selectionSizeLabel_->move(x, y);
    selectionSizeLabel_->show();
    selectionSizeLabel_->raise();
}

void CaptureOverlay::positionToolbar()
{
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

void CaptureOverlay::updateCaptureHintVisibility(const QPoint& point)
{
    captureHint_->setVisible(!captureHint_->geometry().contains(point));
}

void CaptureOverlay::updateCursorForPosition(const QPoint& point)
{
    if (colorPickerActive_) {
        setCursor(Qt::CrossCursor);
        return;
    }
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
    positionSelectionSizeLabel();
    update();
}

void CaptureOverlay::updateWindowTarget(const QPoint& point)
{
    const QRect target = !selection_.isValid()
            && activeHistoryImage_.isNull()
            && annotationTool_ == AnnotationTool::None
            && interaction_ == Interaction::None
        ? windowTargetAt(point)
        : QRect();
    if (hoveredWindowTarget_ == target) {
        return;
    }
    hoveredWindowTarget_ = target;
    positionSelectionSizeLabel();
    update();
}

QRect CaptureOverlay::windowTargetAt(const QPoint& point) const
{
    for (const QRect& target : windowTargets_) {
        if (target.contains(point)) {
            return target;
        }
    }
    return {};
}

} // namespace snipnexs
