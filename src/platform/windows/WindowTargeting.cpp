#include "WindowTargeting.h"

#include "capture/CaptureGeometry.h"

#include <dwmapi.h>
#include <windows.h>

namespace snipnexs {

namespace {

struct TargetContext {
    QRect nativeScreenRect;
    QSize logicalScreenSize;
    QList<QRect> targets;
};

BOOL CALLBACK appendWindowTarget(HWND window, LPARAM parameter)
{
    auto& context = *reinterpret_cast<TargetContext*>(parameter);
    if (!IsWindowVisible(window) || IsIconic(window)) {
        return TRUE;
    }

    DWORD cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(
            window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))
        && cloaked != 0) {
        return TRUE;
    }

    RECT bounds {};
    if (FAILED(DwmGetWindowAttribute(
            window, DWMWA_EXTENDED_FRAME_BOUNDS, &bounds, sizeof(bounds)))
        && !GetWindowRect(window, &bounds)) {
        return TRUE;
    }

    const QRect logicalRect = nativeWindowToLogicalRect(
        QRect(
            bounds.left,
            bounds.top,
            bounds.right - bounds.left,
            bounds.bottom - bounds.top),
        context.nativeScreenRect,
        context.logicalScreenSize);
    if (logicalRect.width() >= 3
        && logicalRect.height() >= 3
        && !context.targets.contains(logicalRect)) {
        context.targets.append(logicalRect);
    }
    return TRUE;
}

}

QList<QRect> visibleTopLevelWindowTargets(
    quintptr monitorHandle, const QSize& logicalScreenSize)
{
    if (monitorHandle == 0 || logicalScreenSize.isEmpty()) {
        return {};
    }

    MONITORINFO monitorInfo {};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (!GetMonitorInfoW(
            reinterpret_cast<HMONITOR>(monitorHandle), &monitorInfo)) {
        return {};
    }

    TargetContext context {
        QRect(
            monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top),
        logicalScreenSize,
        {},
    };
    // ponytail: top-level HWND snapping only; add UI Automation ElementFromPoint
    // when control-level targeting is actually requested.
    EnumWindows(appendWindowTarget, reinterpret_cast<LPARAM>(&context));
    return context.targets;
}

} // namespace snipnexs
