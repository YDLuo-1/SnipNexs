#include "PinWindow.h"

#include <QGuiApplication>
#include <QCursor>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QWheelEvent>

#include <algorithm>

namespace snipnexs {

PinWindow::PinWindow(const QImage& image, QWidget* parent)
    : QWidget(parent)
    , pixmap_(QPixmap::fromImage(image))
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setWindowTitle(QStringLiteral("SnipNexs Pin"));
    setAttribute(Qt::WA_DeleteOnClose);
    setCursor(Qt::OpenHandCursor);

    QSize target = image.size();
    if (target.width() < 120 || target.height() < 80) {
        target.scale(QSize(120, 80), Qt::KeepAspectRatioByExpanding);
    }
    if (QScreen* screen = QGuiApplication::screenAt(QCursor::pos())) {
        const QSize limit = screen->availableGeometry().size() * 0.8;
        target.scale(limit, Qt::KeepAspectRatio);
    }
    resize(target.expandedTo(QSize(120, 80)));
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
    painter.setRenderHint(
        QPainter::SmoothPixmapTransform,
        QSizeF(size()) != pixmap_.deviceIndependentSize());
    painter.drawPixmap(rect(), pixmap_);
}

void PinWindow::wheelEvent(QWheelEvent* event)
{
    resizeBy(event->angleDelta().y() > 0 ? 1.1 : 1.0 / 1.1);
    event->accept();
}

void PinWindow::resizeBy(qreal factor)
{
    QSize target = size() * factor;
    if (target.width() < 120 || target.height() < 80) {
        target.scale(QSize(120, 80), Qt::KeepAspectRatioByExpanding);
    }
    if (QScreen* screen = QGuiApplication::screenAt(frameGeometry().center())) {
        const QSize limit = screen->availableGeometry().size() * 0.95;
        target.scale(limit, Qt::KeepAspectRatio);
    }

    const QPoint center = frameGeometry().center();
    resize(target);
    move(center - rect().center());
}

} // namespace snipnexs
