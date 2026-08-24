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

[[nodiscard]] inline QRect nativeWindowToLogicalRect(
    const QRect& nativeWindowRect,
    const QRect& nativeScreenRect,
    const QSize& logicalScreenSize)
{
    if (!nativeWindowRect.isValid()
        || !nativeScreenRect.isValid()
        || logicalScreenSize.isEmpty()) {
        return {};
    }

    const QRect clipped = nativeWindowRect.intersected(nativeScreenRect);
    if (!clipped.isValid()) {
        return {};
    }

    const qreal scaleX = qreal(logicalScreenSize.width()) / nativeScreenRect.width();
    const qreal scaleY = qreal(logicalScreenSize.height()) / nativeScreenRect.height();
    const int left = qFloor((clipped.x() - nativeScreenRect.x()) * scaleX);
    const int top = qFloor((clipped.y() - nativeScreenRect.y()) * scaleY);
    const int right = qCeil(
        (clipped.x() + clipped.width() - nativeScreenRect.x()) * scaleX);
    const int bottom = qCeil(
        (clipped.y() + clipped.height() - nativeScreenRect.y()) * scaleY);
    return QRect(left, top, right - left, bottom - top)
        .intersected(QRect(QPoint(0, 0), logicalScreenSize));
}

} // namespace snipnexs
