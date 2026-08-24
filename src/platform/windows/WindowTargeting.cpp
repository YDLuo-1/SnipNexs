#include "WindowTargeting.h"

#include "capture/CaptureGeometry.h"

#include <dwmapi.h>
#include <windows.h>

#include <algorithm>
#include <utility>

namespace snipnexs {

namespace {

constexpr qsizetype kMaximumTargets = 512;
constexpr qsizetype kMaximumDetailTargetsPerWindow = 128;
constexpr int kMinimumDetailWidth = 48;
constexpr int kMinimumDetailHeight = 32;

struct TargetContext {
    QRect nativeScreenRect;
    QSize logicalScreenSize;
    QList<QRect> targets;
};

struct ChildTargetContext {
    const TargetContext& targetContext;
    QList<QRect> targets;
};

QRect logicalRect(const RECT& bounds, const TargetContext& context)
{
    return nativeWindowToLogicalRect(
        QRect(
            bounds.left,
            bounds.top,
            bounds.right - bounds.left,
            bounds.bottom - bounds.top),
        context.nativeScreenRect,
        context.logicalScreenSize);
}

BOOL CALLBACK appendChildTarget(HWND window, LPARAM parameter)
{
    auto& context = *reinterpret_cast<ChildTargetContext*>(parameter);
    if (context.targets.size() >= kMaximumDetailTargetsPerWindow) {
        return FALSE;
    }
    if (!IsWindowVisible(window) || IsIconic(window)) {
        return TRUE;
    }

    RECT bounds {};
    if (!GetWindowRect(window, &bounds)) {
        return TRUE;
    }
    const QRect target = logicalRect(bounds, context.targetContext);
    if (target.width() >= kMinimumDetailWidth
        && target.height() >= kMinimumDetailHeight
        && !context.targets.contains(target)) {
        context.targets.append(target);
    }
    return TRUE;
}

QRect clientTarget(HWND window, const TargetContext& context)
{
    RECT client {};
    if (!GetClientRect(window, &client)) {
        return {};
    }
    POINT topLeft {client.left, client.top};
    POINT bottomRight {client.right, client.bottom};
    if (!ClientToScreen(window, &topLeft)
        || !ClientToScreen(window, &bottomRight)) {
        return {};
    }
    const RECT screenClient {
        topLeft.x,
        topLeft.y,
        bottomRight.x,
        bottomRight.y,
    };
    return logicalRect(screenClient, context);
}

BOOL CALLBACK appendWindowTarget(HWND window, LPARAM parameter)
{
    auto& context = *reinterpret_cast<TargetContext*>(parameter);
    if (context.targets.size() >= kMaximumTargets) {
        return FALSE;
    }
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

    const QRect frameTarget = logicalRect(bounds, context);
    if (frameTarget.width() < 3 || frameTarget.height() < 3) {
        return TRUE;
    }

    ChildTargetContext childContext {context, {}};
    EnumChildWindows(
        window, appendChildTarget, reinterpret_cast<LPARAM>(&childContext));
    const QList<QRect> group = prioritizeWindowTargetGroup(
        std::move(childContext.targets), clientTarget(window, context), frameTarget);
    for (const QRect& target : group) {
        if (!context.targets.contains(target)) {
            context.targets.append(target);
            if (context.targets.size() >= kMaximumTargets) {
                break;
            }
        }
    }
    return TRUE;
}

}

QList<QRect> prioritizeWindowTargetGroup(
    QList<QRect> detailTargets, const QRect& clientTarget, const QRect& frameTarget)
{
    detailTargets.removeIf([](const QRect& target) { return !target.isValid(); });
    std::stable_sort(
        detailTargets.begin(), detailTargets.end(),
        [](const QRect& left, const QRect& right) {
            const qint64 leftArea = qint64(left.width()) * left.height();
            const qint64 rightArea = qint64(right.width()) * right.height();
            return leftArea < rightArea;
        });

    QList<QRect> result;
    const auto appendUnique = [&result](const QRect& target) {
        if (target.isValid() && !result.contains(target)) {
            result.append(target);
        }
    };
    for (const QRect& target : detailTargets) {
        appendUnique(target);
    }
    appendUnique(clientTarget);
    appendUnique(frameTarget);
    return result;
}

QList<QRect> visibleWindowTargets(
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
    EnumWindows(appendWindowTarget, reinterpret_cast<LPARAM>(&context));
    return context.targets;
}

} // namespace snipnexs
