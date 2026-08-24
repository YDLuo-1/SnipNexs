#include "CaptureOverlay.h"

#include "CaptureGeometry.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCursor>
#include <QFrame>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>

#include <algorithm>
#include <array>
#include <utility>

namespace snipnexs {

namespace {

constexpr int kHandleSize = 8;
constexpr int kHandleHitPadding = 3;
constexpr int kMinimumSelectionSize = 3;

enum class ToolbarIcon {
    Pen,
    Rectangle,
    Arrow,
    Undo,
    Redo,
    Ocr,
    Pin,
    Record,
    Copy,
    Save,
    Cancel,
};

QPixmap drawToolbarIcon(ToolbarIcon icon, const QColor& color)
{
    QPixmap pixmap(40, 40);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 3.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    switch (icon) {
    case ToolbarIcon::Pen:
        painter.drawLine(QPointF(10, 30), QPointF(27, 13));
        painter.drawLine(QPointF(8, 32), QPointF(15, 29));
        painter.drawLine(QPointF(25, 11), QPointF(29, 15));
        break;
    case ToolbarIcon::Rectangle:
        painter.drawRoundedRect(QRectF(8, 10, 24, 20), 2, 2);
        break;
    case ToolbarIcon::Arrow:
        painter.drawLine(QPointF(9, 31), QPointF(30, 10));
        painter.drawLine(QPointF(20, 10), QPointF(30, 10));
        painter.drawLine(QPointF(30, 10), QPointF(30, 20));
        break;
    case ToolbarIcon::Undo:
    case ToolbarIcon::Redo: {
        painter.save();
        if (icon == ToolbarIcon::Redo) {
            painter.translate(40, 0);
            painter.scale(-1, 1);
        }
        QPainterPath path;
        path.moveTo(32, 28);
        path.cubicTo(30, 15, 18, 12, 9, 21);
        painter.drawPath(path);
        painter.drawLine(QPointF(9, 21), QPointF(10, 12));
        painter.drawLine(QPointF(9, 21), QPointF(18, 20));
        painter.restore();
        break;
    }
    case ToolbarIcon::Ocr: {
        QFont font = painter.font();
        font.setPixelSize(17);
        font.setWeight(QFont::DemiBold);
        painter.setFont(font);
        painter.setPen(color);
        painter.drawText(QRect(4, 5, 32, 30), Qt::AlignCenter, QStringLiteral("Aa"));
        break;
    }
    case ToolbarIcon::Pin:
        painter.drawLine(QPointF(20, 21), QPointF(20, 33));
        painter.drawLine(QPointF(15, 31), QPointF(20, 36));
        painter.drawLine(QPointF(20, 36), QPointF(25, 31));
        painter.drawRoundedRect(QRectF(11, 8, 18, 14), 2, 2);
        painter.drawLine(QPointF(14, 22), QPointF(26, 22));
        break;
    case ToolbarIcon::Record:
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawEllipse(QPointF(20, 20), 9, 9);
        break;
    case ToolbarIcon::Copy:
        painter.drawRoundedRect(QRectF(13, 9, 19, 22), 2, 2);
        painter.drawRoundedRect(QRectF(8, 14, 19, 19), 2, 2);
        break;
    case ToolbarIcon::Save:
        painter.drawRoundedRect(QRectF(9, 7, 22, 26), 2, 2);
        painter.drawRect(QRectF(14, 8, 12, 8));
        painter.drawRect(QRectF(14, 23, 12, 9));
        break;
    case ToolbarIcon::Cancel:
        painter.drawLine(QPointF(11, 11), QPointF(29, 29));
        painter.drawLine(QPointF(29, 11), QPointF(11, 29));
        break;
    }
    painter.end();
    return pixmap;
}

QIcon makeToolbarIcon(ToolbarIcon icon, bool darkByDefault = false)
{
    QIcon result;
    const QColor light(234, 240, 245);
    const QColor dark(9, 40, 36);
    result.addPixmap(drawToolbarIcon(icon, darkByDefault ? dark : light), QIcon::Normal, QIcon::Off);
    result.addPixmap(drawToolbarIcon(icon, dark), QIcon::Normal, QIcon::On);
    return result;
}

void configureToolbarButton(
    QPushButton* button,
    ToolbarIcon icon,
    const QString& tooltip,
    bool darkByDefault = false)
{
    button->setIcon(makeToolbarIcon(icon, darkByDefault));
    button->setIconSize(QSize(20, 20));
    button->setToolTip(tooltip);
    button->setAccessibleName(tooltip);
    button->setFixedSize(36, 32);
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
    layout->setContentsMargins(9, 7, 9, 7);
    layout->setSpacing(7);

    sizeLabel_ = new QLabel(toolbar_);
    auto* penButton = new QPushButton(toolbar_);
    auto* rectangleButton = new QPushButton(toolbar_);
    auto* arrowButton = new QPushButton(toolbar_);
    undoButton_ = new QPushButton(toolbar_);
    redoButton_ = new QPushButton(toolbar_);
    auto* ocrButton = new QPushButton(toolbar_);
    ocrButton->setObjectName(QStringLiteral("ocrButton"));
    auto* pinButton = new QPushButton(toolbar_);
    recordButton_ = new QPushButton(toolbar_);
    recordButton_->setObjectName(QStringLiteral("recordButton"));
    auto* copyButton = new QPushButton(toolbar_);
    auto* saveButton = new QPushButton(toolbar_);
    auto* cancelButton = new QPushButton(toolbar_);
    copyButton->setObjectName(QStringLiteral("accentButton"));
    configureToolbarButton(penButton, ToolbarIcon::Pen, tr("画笔"));
    configureToolbarButton(rectangleButton, ToolbarIcon::Rectangle, tr("矩形"));
    configureToolbarButton(arrowButton, ToolbarIcon::Arrow, tr("箭头"));
    configureToolbarButton(undoButton_, ToolbarIcon::Undo, tr("撤销"));
    configureToolbarButton(redoButton_, ToolbarIcon::Redo, tr("重做"));
    configureToolbarButton(ocrButton, ToolbarIcon::Ocr, tr("识字"));
    configureToolbarButton(pinButton, ToolbarIcon::Pin, tr("贴图"));
    configureToolbarButton(recordButton_, ToolbarIcon::Record, tr("录屏"));
    configureToolbarButton(copyButton, ToolbarIcon::Copy, tr("复制"), true);
    configureToolbarButton(saveButton, ToolbarIcon::Save, tr("保存"));
    configureToolbarButton(cancelButton, ToolbarIcon::Cancel, tr("取消"));
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
        QPushButton { padding: 0; color: #eaf0f5; background: #27323e; border: 0; border-radius: 5px; }
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
    hoveredWindowTarget_ = {};
    pressedWindowTarget_ = {};
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

void CaptureOverlay::setWindowTargets(QList<QRect> targets)
{
    windowTargets_ = std::move(targets);
    updateWindowTarget(mapFromGlobal(QCursor::pos()));
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
    const QPoint point = event->position().toPoint();
    updateCaptureHintVisibility(point);
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
        emit canceled();
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }

    const QPoint point = event->position().toPoint();
    updateCaptureHintVisibility(point);
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
}

void CaptureOverlay::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    positionCaptureHint();
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
        hoveredWindowTarget_ = {};
        toolbar_->hide();
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

void CaptureOverlay::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    positionCaptureHint();
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

void CaptureOverlay::positionCaptureHint()
{
    captureHint_->adjustSize();
    captureHint_->move(16, std::max(16, height() - captureHint_->height() - 16));
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

void CaptureOverlay::updateCaptureHintVisibility(const QPoint& point)
{
    captureHint_->setVisible(!captureHint_->geometry().contains(point));
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
