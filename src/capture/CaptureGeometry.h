#pragma once

#include <QRect>
#include <QSize>
#include <QtMath>

namespace snipnexs {

[[nodiscard]] inline QRect logicalToPixelRect(
    const QRect& logicalRect, qreal devicePixelRatio, const QSize& pixelSize)
{
    if (!logicalRect.isValid() || devicePixelRatio <= 0.0 || pixelSize.isEmpty()) {
        return {};
    }

    const int left = qFloor(logicalRect.x() * devicePixelRatio);
    const int top = qFloor(logicalRect.y() * devicePixelRatio);
    const int right = qCeil((logicalRect.x() + logicalRect.width()) * devicePixelRatio);
    const int bottom = qCeil((logicalRect.y() + logicalRect.height()) * devicePixelRatio);
    return QRect(left, top, right - left, bottom - top)
        .intersected(QRect(QPoint(0, 0), pixelSize));
}

} // namespace snipnexs
