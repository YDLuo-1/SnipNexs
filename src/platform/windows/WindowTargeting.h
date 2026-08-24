#pragma once

#include <QList>
#include <QRect>
#include <QSize>

namespace snipnexs {

[[nodiscard]] QList<QRect> visibleTopLevelWindowTargets(
    quintptr monitorHandle, const QSize& logicalScreenSize);

} // namespace snipnexs
