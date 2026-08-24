#include "CaptureExclusion.h"

#include <QWidget>

#include <windows.h>

namespace snipnexs {

bool setWindowExcludedFromCapture(QWidget& window, bool excluded)
{
    const auto handle = reinterpret_cast<HWND>(window.winId());
    return SetWindowDisplayAffinity(
               handle,
               excluded ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE)
        != FALSE;
}

} // namespace snipnexs
