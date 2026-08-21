#include "AppIcon.h"

#include <QPainter>
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
        painter.setBrush(QColor(22, 28, 36));
        painter.drawRoundedRect(pixmap.rect().adjusted(1, 1, -1, -1), size * 0.2, size * 0.2);

        QPen stroke(QColor(57, 208, 190), qMax(2.0, size * 0.09), Qt::SolidLine, Qt::RoundCap);
        painter.setPen(stroke);
        const qreal inset = size * 0.29;
        painter.drawLine(QPointF(inset, inset), QPointF(size - inset, size - inset));
        painter.drawLine(QPointF(size - inset, inset), QPointF(inset, size - inset));

        icon.addPixmap(pixmap);
    }
    return icon;
}

} // namespace snipnexs
