#include "PinWindow.h"

#include <QContextMenuEvent>
#include <QCursor>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QScreen>
#include <QToolButton>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace snipnexs {

namespace {

constexpr int kShadowMargin = 16;
constexpr qreal kMinimumImageSize = 24.0;

enum class ToolbarIcon {
    Copy,
    Save,
    Close,
};

QIcon makeToolbarIcon(ToolbarIcon kind)
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(245, 248, 252), 1.8, Qt::SolidLine, Qt::RoundCap,
        Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    if (kind == ToolbarIcon::Copy) {
        painter.drawRoundedRect(QRectF(7.0, 5.0, 10.0, 12.0), 1.5, 1.5);
        painter.drawRoundedRect(QRectF(4.0, 8.0, 10.0, 10.0), 1.5, 1.5);
    } else if (kind == ToolbarIcon::Save) {
        painter.drawRoundedRect(QRectF(4.0, 3.0, 14.0, 16.0), 1.5, 1.5);
        painter.drawLine(QPointF(7.0, 4.0), QPointF(7.0, 10.0));
        painter.drawLine(QPointF(15.0, 4.0), QPointF(15.0, 10.0));
        painter.drawRoundedRect(QRectF(7.0, 13.0, 8.0, 4.0), 0.8, 0.8);
    } else {
        painter.drawLine(QPointF(5.0, 5.0), QPointF(17.0, 17.0));
        painter.drawLine(QPointF(17.0, 5.0), QPointF(5.0, 17.0));
    }
    return QIcon(pixmap);
}

}

PinWindow::PinWindow(const QImage& image, QWidget* parent)
    : QWidget(parent)
    , sourceImage_(image)
    , pixmap_(QPixmap::fromImage(image))
{
    pixmap_.setDevicePixelRatio(image.devicePixelRatio() > 0.0
        ? image.devicePixelRatio()
        : 1.0);

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setWindowTitle(QStringLiteral("SnipNexs Pin"));
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    setCursor(Qt::OpenHandCursor);

    toolbar_ = new QFrame(this);
    toolbar_->setObjectName(QStringLiteral("pinToolbar"));
    toolbar_->setStyleSheet(QStringLiteral(
        "QFrame#pinToolbar {"
        " background: rgba(24, 30, 40, 235);"
        " border: 1px solid rgba(255, 255, 255, 90);"
        " border-radius: 7px;"
        "}"
        "QToolButton { border: 0; border-radius: 5px; padding: 4px; }"
        "QToolButton:hover { background: rgba(80, 160, 220, 180); }"
        "QToolButton:pressed { background: rgba(40, 120, 190, 210); }"));
    auto* toolbarLayout = new QHBoxLayout(toolbar_);
    toolbarLayout->setContentsMargins(4, 4, 4, 4);
    toolbarLayout->setSpacing(2);

    auto addButton = [this, toolbarLayout](
        ToolbarIcon icon, const QString& toolTip, const char* objectName) {
        auto* button = new QToolButton(toolbar_);
        button->setObjectName(QString::fromLatin1(objectName));
        button->setIcon(makeToolbarIcon(icon));
        button->setIconSize(QSize(20, 20));
        button->setFixedSize(32, 32);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        button->setToolTip(toolTip);
        button->setAccessibleName(toolTip);
        toolbarLayout->addWidget(button);
        return button;
    };
    QToolButton* copyButton = addButton(
        ToolbarIcon::Copy, tr("复制图像"), "pinCopyButton");
    QToolButton* saveButton = addButton(
        ToolbarIcon::Save, tr("图像另存为..."), "pinSaveButton");
    QToolButton* closeButton = addButton(
        ToolbarIcon::Close, tr("关闭贴图"), "pinCloseButton");
    connect(copyButton, &QToolButton::clicked, this, [this]() {
        emit copyRequested(sourceImage_);
    });
    connect(saveButton, &QToolButton::clicked, this, [this]() {
        emit saveRequested(sourceImage_);
    });
    connect(closeButton, &QToolButton::clicked, this, &PinWindow::close);
    toolbar_->hide();

    const QSize target = pixmap_.deviceIndependentSize().toSize();
    resize(target + QSize(kShadowMargin * 2, kShadowMargin * 2));
    positionToolbar();
}

void PinWindow::moveImageTopLeft(const QPoint& globalTopLeft)
{
    QPoint imageTopLeft = globalTopLeft;
    if (QScreen* screen = QGuiApplication::screenAt(globalTopLeft)) {
        const QRect available = screen->availableGeometry();
        const QSize imageSize = imageRect().size().toSize();
        imageTopLeft.setX(std::clamp(
            imageTopLeft.x(),
            available.left(),
            std::max(available.left(), available.right() - imageSize.width() + 1)));
        imageTopLeft.setY(std::clamp(
            imageTopLeft.y(),
            available.top(),
            std::max(available.top(), available.bottom() - imageSize.height() + 1)));
    }
    move(imageTopLeft - QPoint(kShadowMargin, kShadowMargin));
}

void PinWindow::contextMenuEvent(QContextMenuEvent* event)
{
    auto* menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    QAction* copyAction = menu->addAction(tr("复制图像"));
    QAction* saveAction = menu->addAction(tr("图像另存为..."));
    QAction* toolbarAction = menu->addAction(
        toolbar_ != nullptr && toolbar_->isVisible()
            ? tr("隐藏工具条")
            : tr("显示工具条"));
    menu->addSeparator();
    QAction* closeAction = menu->addAction(tr("关闭贴图"));

    connect(copyAction, &QAction::triggered, this, [this]() {
        emit copyRequested(sourceImage_);
    });
    connect(saveAction, &QAction::triggered, this, [this]() {
        emit saveRequested(sourceImage_);
    });
    connect(toolbarAction, &QAction::triggered, this, [this]() {
        setToolbarVisible(toolbar_ == nullptr || !toolbar_->isVisible());
    });
    connect(closeAction, &QAction::triggered, this, &PinWindow::close);
    menu->popup(event->globalPos());
    event->accept();
}

void PinWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        close();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void PinWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (moving_) {
        move(event->globalPosition().toPoint() - dragOffset_);
    }
}

void PinWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        moving_ = true;
        dragOffset_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
        setCursor(Qt::ClosedHandCursor);
    }
}

void PinWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        moving_ = false;
        setCursor(Qt::OpenHandCursor);
    }
}

void PinWindow::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF target = imageRect();
    painter.setPen(Qt::NoPen);
    for (int spread = 10; spread >= 2; spread -= 2) {
        painter.setBrush(QColor(0, 0, 0, 3 + (10 - spread)));
        painter.drawRoundedRect(
            target.adjusted(-spread, -spread, spread, spread),
            6 + spread,
            6 + spread);
    }

    painter.setRenderHint(
        QPainter::SmoothPixmapTransform,
        target.size() != pixmap_.deviceIndependentSize());
    painter.drawPixmap(
        target, pixmap_, QRectF(pixmap_.rect()));

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(255, 255, 255, 70), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(target.adjusted(0.5, 0.5, -0.5, -0.5));
}

void PinWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    positionToolbar();
}

void PinWindow::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() == 0) {
        event->ignore();
        return;
    }

    const qreal steps = event->angleDelta().y() / 120.0;
    resizeBy(std::pow(1.1, steps), event->position());
    event->accept();
}

void PinWindow::resizeBy(qreal factor, const QPointF& anchorPosition)
{
    const QRectF current = imageRect();
    if (!current.isValid() || factor <= 0.0) {
        return;
    }

    const QPointF normalized(
        std::clamp((anchorPosition.x() - current.left()) / current.width(), 0.0, 1.0),
        std::clamp((anchorPosition.y() - current.top()) / current.height(), 0.0, 1.0));
    const QPoint globalAnchor = mapToGlobal(anchorPosition.toPoint());

    const qreal minimumFactor = std::max(
        kMinimumImageSize / current.width(),
        kMinimumImageSize / current.height());
    factor = std::max(factor, minimumFactor);

    if (QScreen* screen = QGuiApplication::screenAt(frameGeometry().center())) {
        const QSizeF limit = QSizeF(screen->availableGeometry().size()) * 0.95
            - QSizeF(kShadowMargin * 2, kShadowMargin * 2);
        const qreal maximumFactor = std::min(
            limit.width() / current.width(),
            limit.height() / current.height());
        factor = std::min(factor, std::max<qreal>(1.0, maximumFactor));
    }

    const QSizeF target = current.size() * factor;
    resize(
        qMax(1, qRound(target.width())) + kShadowMargin * 2,
        qMax(1, qRound(target.height())) + kShadowMargin * 2);

    const QRectF resizedImage = imageRect();
    const QPointF resizedAnchor(
        resizedImage.left() + normalized.x() * resizedImage.width(),
        resizedImage.top() + normalized.y() * resizedImage.height());
    move(globalAnchor - resizedAnchor.toPoint());
}

QRectF PinWindow::imageRect() const
{
    return QRectF(rect()).adjusted(
        kShadowMargin,
        kShadowMargin,
        -kShadowMargin,
        -kShadowMargin);
}

void PinWindow::positionToolbar()
{
    if (toolbar_ == nullptr) {
        return;
    }
    toolbar_->adjustSize();
    const int x = std::max(8, (width() - toolbar_->width()) / 2);
    const int y = std::max(8, height() - toolbar_->height() - 8);
    toolbar_->move(x, y);
    toolbar_->raise();
}

void PinWindow::setToolbarVisible(bool visible)
{
    if (toolbar_ == nullptr) {
        return;
    }
    toolbar_->setVisible(visible);
    positionToolbar();
}

} // namespace snipnexs
