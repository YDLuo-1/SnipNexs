#include "GlobalHotkey.h"

#include <QCoreApplication>

#include <windows.h>

namespace snipnexs {

GlobalHotkey::GlobalHotkey(QObject* parent)
    : QObject(parent)
{
    QCoreApplication::instance()->installNativeEventFilter(this);
}

GlobalHotkey::~GlobalHotkey()
{
    if (registered_) {
        UnregisterHotKey(nullptr, kHotkeyId);
    }
    QCoreApplication::instance()->removeNativeEventFilter(this);
}

bool GlobalHotkey::registerCaptureShortcut()
{
    if (registered_) {
        return true;
    }
    registered_ = RegisterHotKey(
        nullptr,
        kHotkeyId,
        MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT,
        static_cast<UINT>('A')) != FALSE;
    return registered_;
}

bool GlobalHotkey::nativeEventFilter(
    const QByteArray&, void* message, qintptr*)
{
    const auto* nativeMessage = static_cast<MSG*>(message);
    if (nativeMessage->message == WM_HOTKEY
        && nativeMessage->wParam == static_cast<WPARAM>(kHotkeyId)) {
        emit activated();
        return true;
    }
    return false;
}

} // namespace snipnexs
