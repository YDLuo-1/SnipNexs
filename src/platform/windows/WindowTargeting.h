#pragma once

#include <QList>
#include <QRect>
#include <QSize>

namespace snipnexs {

[[nodiscard]] QList<QRect> prioritizeWindowTargetGroup(
    QList<QRect> detailTargets, const QRect& clientTarget, const QRect& frameTarget);

[[nodiscard]] QList<QRect> visibleWindowTargets(
    quintptr monitorHandle, const QSize& logicalScreenSize);

} // namespace snipnexs
