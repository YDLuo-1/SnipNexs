#include "AppIcon.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace snipnexs {

QIcon createAppIcon()
{
    QIcon icon;
    for (const int size : {16, 24, 32, 48, 64, 128}) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(13, 25, 33));
        painter.drawRoundedRect(
            pixmap.rect().adjusted(1, 1, -1, -1), size * 0.22, size * 0.22);

        const qreal inset = size * 0.20;
        const qreal arm = size * 0.16;
        QPen cropPen(
            QColor(55, 214, 192), qMax(1.0, size * 0.055),
            Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(cropPen);
        painter.drawLine(QPointF(inset, inset + arm), QPointF(inset, inset));
        painter.drawLine(QPointF(inset, inset), QPointF(inset + arm, inset));
        painter.drawLine(
            QPointF(size - inset - arm, inset), QPointF(size - inset, inset));
        painter.drawLine(
            QPointF(size - inset, inset), QPointF(size - inset, inset + arm));
        painter.drawLine(
            QPointF(inset, size - inset - arm), QPointF(inset, size - inset));
        painter.drawLine(
            QPointF(inset, size - inset), QPointF(inset + arm, size - inset));
        painter.drawLine(
            QPointF(size - inset - arm, size - inset),
            QPointF(size - inset, size - inset));
        painter.drawLine(
            QPointF(size - inset, size - inset),
            QPointF(size - inset, size - inset - arm));

        QPainterPath letter;
        letter.moveTo(size * 0.36, size * 0.67);
        letter.lineTo(size * 0.36, size * 0.34);
        letter.lineTo(size * 0.64, size * 0.67);
        letter.lineTo(size * 0.64, size * 0.34);
        painter.setPen(QPen(
            QColor(245, 249, 252), qMax(1.4, size * 0.075),
            Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(letter);

        icon.addPixmap(pixmap);
    }
    return icon;
}

} // namespace snipnexs
