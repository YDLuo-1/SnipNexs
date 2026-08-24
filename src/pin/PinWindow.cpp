#include "PinWindow.h"

#include <QCursor>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QScreen>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace snipnexs {

namespace {

constexpr int kShadowMargin = 16;
constexpr qreal kMinimumImageSize = 24.0;

}

PinWindow::PinWindow(const QImage& image, QWidget* parent)
    : QWidget(parent)
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

    const QSize target = pixmap_.deviceIndependentSize().toSize();
    resize(target + QSize(kShadowMargin * 2, kShadowMargin * 2));
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
        close();
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
        target, pixmap_, QRectF(QPointF(0, 0), pixmap_.deviceIndependentSize()));

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(255, 255, 255, 70), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(target.adjusted(0.5, 0.5, -0.5, -0.5));
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

} // namespace snipnexs
