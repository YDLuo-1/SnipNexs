#include "capture/ToolbarIcons.h"

#include <QPainter>
#include <QPainterPath>

namespace snipnexs {

namespace {

constexpr qreal kStroke = 3.4;

QPen iconPen(const QColor& color)
{
    return QPen(color, kStroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
}

void fillPath(QPainter& painter, const QPainterPath& path, const QColor& color)
{
    const QPen pen = painter.pen();
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPath(path);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
}

void drawPen(QPainter& painter, const QColor& color)
{
    painter.save();
    painter.translate(24, 24);
    painter.rotate(45);

    QPainterPath tip;
    tip.moveTo(-6, 6);
    tip.lineTo(6, 6);
    tip.lineTo(0, 15);
    tip.closeSubpath();

    painter.drawRoundedRect(QRectF(-6, -15, 12, 21), 3.5, 3.5);
    painter.drawPath(tip);
    painter.drawLine(QPointF(-6, -10), QPointF(6, -10));
    painter.restore();
}

void drawArrow(QPainter& painter, const QColor& color)
{
    QPainterPath head;
    head.moveTo(39, 9);
    head.lineTo(39, 23);
    head.lineTo(25, 9);
    head.closeSubpath();

    painter.drawLine(QPointF(11, 37), QPointF(32.5, 15.5));
    fillPath(painter, head, color);
}

void drawColorPicker(QPainter& painter, const QColor& color)
{
    painter.save();
    painter.translate(24, 24);
    painter.rotate(-45);

    QPainterPath tip;
    tip.moveTo(-3, 5.5);
    tip.lineTo(3, 5.5);
    tip.lineTo(0, 13.5);
    tip.closeSubpath();

    painter.drawEllipse(QPointF(0, -14), 6, 6);
    painter.drawRoundedRect(QRectF(-3, -8.5, 6, 14), 3, 3);
    painter.drawPath(tip);
    painter.restore();
}

void drawUndo(QPainter& painter, const QColor& color)
{
    QPainterPath curve;
    curve.moveTo(38, 34);
    curve.cubicTo(36, 20, 26, 12, 15, 15);

    QPainterPath head;
    head.moveTo(15, 10.5);
    head.lineTo(15, 21.5);
    head.lineTo(6.5, 16);
    head.closeSubpath();

    painter.drawPath(curve);
    fillPath(painter, head, color);
}

void drawOcr(QPainter& painter, const QColor& color)
{
    QPainterPath corner;
    corner.moveTo(8, 17);
    corner.lineTo(8, 9);
    corner.lineTo(16, 9);
    painter.drawPath(corner);

    corner = QPainterPath();
    corner.moveTo(32, 9);
    corner.lineTo(40, 9);
    corner.lineTo(40, 17);
    painter.drawPath(corner);

    corner = QPainterPath();
    corner.moveTo(40, 31);
    corner.lineTo(40, 39);
    corner.lineTo(32, 39);
    painter.drawPath(corner);

    corner = QPainterPath();
    corner.moveTo(16, 39);
    corner.lineTo(8, 39);
    corner.lineTo(8, 31);
    painter.drawPath(corner);

    painter.drawLine(QPointF(15, 19), QPointF(33, 19));
    painter.drawLine(QPointF(15, 24.2), QPointF(33, 24.2));
    painter.drawLine(QPointF(15, 29.4), QPointF(33, 29.4));
}

void drawPin(QPainter& painter, const QColor& color)
{
    QPainterPath flange;
    flange.moveTo(18, 15);
    flange.lineTo(30, 15);
    flange.lineTo(33, 23);
    flange.lineTo(15, 23);
    flange.closeSubpath();

    painter.drawRoundedRect(QRectF(16, 8, 16, 7), 2, 2);
    painter.drawPath(flange);
    painter.drawLine(QPointF(24, 23), QPointF(24, 38));
}

void drawRecord(QPainter& painter, const QColor& color)
{
    QPainterPath flap;
    flap.moveTo(33.5, 20.5);
    flap.lineTo(40, 15.5);
    flap.lineTo(40, 32.5);
    flap.lineTo(33.5, 27.5);
    flap.closeSubpath();

    painter.drawRoundedRect(QRectF(8, 14, 22, 20), 4.5, 4.5);
    fillPath(painter, flap, color);
}

void drawCopy(QPainter& painter, const QColor& color)
{
    QPainterPath front;
    front.addRoundedRect(QRectF(18, 10, 21, 25), 3.5, 3.5);
    QPainterPath back;
    back.addRoundedRect(QRectF(9, 13, 21, 25), 3.5, 3.5);

    painter.drawPath(back.subtracted(front));
    painter.drawPath(front);
}

void drawSave(QPainter& painter, const QColor& color)
{
    QPainterPath head;
    head.moveTo(17.5, 21);
    head.lineTo(30.5, 21);
    head.lineTo(24, 28.5);
    head.closeSubpath();

    QPainterPath tray;
    tray.moveTo(10, 29);
    tray.lineTo(10, 37);
    tray.lineTo(38, 37);
    tray.lineTo(38, 29);

    painter.drawLine(QPointF(24, 9), QPointF(24, 21.5));
    fillPath(painter, head, color);
    painter.drawPath(tray);
}

} // namespace

QPixmap drawToolbarIcon(ToolbarIcon icon, const QColor& color)
{
    QPixmap pixmap(48, 48);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(iconPen(color));
    painter.setBrush(Qt::NoBrush);

    switch (icon) {
    case ToolbarIcon::Pen:
        drawPen(painter, color);
        break;
    case ToolbarIcon::Rectangle:
        painter.drawRoundedRect(QRectF(9, 13, 30, 22), 4, 4);
        break;
    case ToolbarIcon::Arrow:
        drawArrow(painter, color);
        break;
    case ToolbarIcon::Text:
        painter.drawLine(QPointF(12, 12), QPointF(36, 12));
        painter.drawLine(QPointF(24, 12), QPointF(24, 36));
        painter.drawLine(QPointF(18, 36), QPointF(30, 36));
        break;
    case ToolbarIcon::ColorPicker:
        drawColorPicker(painter, color);
        break;
    case ToolbarIcon::Undo:
    case ToolbarIcon::Redo:
        if (icon == ToolbarIcon::Redo) {
            painter.translate(48, 0);
            painter.scale(-1, 1);
        }
        drawUndo(painter, color);
        break;
    case ToolbarIcon::Ocr:
        drawOcr(painter, color);
        break;
    case ToolbarIcon::Pin:
        drawPin(painter, color);
        break;
    case ToolbarIcon::Record:
        drawRecord(painter, color);
        break;
    case ToolbarIcon::Copy:
        drawCopy(painter, color);
        break;
    case ToolbarIcon::Save:
        drawSave(painter, color);
        break;
    case ToolbarIcon::Cancel:
        painter.drawLine(QPointF(15, 15), QPointF(33, 33));
        painter.drawLine(QPointF(33, 15), QPointF(15, 33));
        break;
    }
    painter.end();
    return pixmap;
}

QIcon makeToolbarIcon(ToolbarIcon icon, bool onDarkBackground)
{
    QIcon result;
    const QColor normal = onDarkBackground
        ? QColor(240, 244, 249)
        : QColor(53, 65, 76);
    const QColor active = onDarkBackground ? QColor(255, 255, 255) : QColor(20, 29, 37);
    const QColor disabled = onDarkBackground
        ? QColor(240, 244, 249, 90)
        : QColor(158, 168, 177);
    result.addPixmap(drawToolbarIcon(icon, normal), QIcon::Normal, QIcon::Off);
    result.addPixmap(drawToolbarIcon(icon, active), QIcon::Active, QIcon::Off);
    result.addPixmap(drawToolbarIcon(icon, disabled), QIcon::Disabled, QIcon::Off);
    result.addPixmap(drawToolbarIcon(icon, QColor(255, 255, 255)), QIcon::Normal, QIcon::On);
    result.addPixmap(drawToolbarIcon(icon, QColor(255, 255, 255)), QIcon::Active, QIcon::On);
    return result;
}

} // namespace snipnexs
