#include "NativeTrayIcon.h"

#include <QCursor>
#include <QGuiApplication>
#include <QImage>
#include <QPixmap>
#include <QScreen>

#include <Windows.h>

#include <cstring>
#include <vector>

namespace snipnexs {

namespace {

constexpr UINT kCallbackMessage = WM_APP + 1;
constexpr UINT kIconId = 1;

UINT taskbarCreatedMessage()
{
    static const UINT message = RegisterWindowMessageW(L"TaskbarCreated");
    return message;
}

HICON iconToHICON(const QIcon& icon)
{
    if (icon.isNull()) {
        return nullptr;
    }
    const int smallSize = GetSystemMetrics(SM_CXSMICON);
    const QScreen* screen = QGuiApplication::primaryScreen();
    const qreal ratio = screen != nullptr ? screen->devicePixelRatio() : 1.0;
    const int wanted = qBound(16, qRound(smallSize * ratio), 64);
    QPixmap pixmap = icon.pixmap(QSize(wanted, wanted));
    if (pixmap.isNull()) {
        pixmap = icon.pixmap(QSize(qMax(16, smallSize), qMax(16, smallSize)));
    }
    if (pixmap.isNull()) {
        return nullptr;
    }
    const QImage image =
        pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    if (image.width() <= 0 || image.height() <= 0) {
        return nullptr;
    }

    // 32-bpp color plane straight from the ARGB32 buffer plus an all-opaque
    // AND mask; the shell honors per-pixel alpha in this configuration.
    HBITMAP color = CreateBitmap(
        image.width(), image.height(), 1, 32, image.constBits());
    const std::vector<BYTE> mask(
        (static_cast<std::size_t>(image.width()) * image.height() + 7) / 8, 0);
    HBITMAP maskBitmap =
        CreateBitmap(image.width(), image.height(), 1, 1, mask.data());
    ICONINFO info{};
    info.fIcon = TRUE;
    info.hbmColor = color;
    info.hbmMask = maskBitmap;
    HICON result = CreateIconIndirect(&info);
    if (color != nullptr) {
        DeleteObject(color);
    }
    if (maskBitmap != nullptr) {
        DeleteObject(maskBitmap);
    }
    return result;
}

} // namespace

class NativeTrayIcon::Impl
{
public:
    Impl(NativeTrayIcon* owner, const QIcon& icon)
        : owner_(owner)
        , icon_(iconToHICON(icon))
    {
    }

    ~Impl()
    {
        if (visible_) {
            NOTIFYICONDATAW data{};
            fillIdentification(data);
            Shell_NotifyIconW(NIM_DELETE, &data);
        }
        if (window_ != nullptr) {
            DestroyWindow(window_);
        }
        if (icon_ != nullptr) {
            DestroyIcon(icon_);
        }
    }

    bool ensureWindow()
    {
        if (window_ != nullptr) {
            return true;
        }
        static const ATOM atom = [] {
            WNDCLASSW description{};
            description.lpfnWndProc = &NativeTrayIcon::Impl::windowProc;
            description.hInstance = GetModuleHandleW(nullptr);
            description.lpszClassName = L"SnipNexsNativeTrayIcon";
            return RegisterClassW(&description);
        }();
        if (atom == 0) {
            return false;
        }
        window_ = CreateWindowExW(WS_EX_TOOLWINDOW, reinterpret_cast<LPCWSTR>(atom),
            L"SnipNexs tray icon", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
            GetModuleHandleW(nullptr), this);
        return window_ != nullptr;
    }

    void fillIdentification(NOTIFYICONDATAW& data) const
    {
        data.cbSize = sizeof(data);
        data.hWnd = window_;
        data.uID = kIconId;
    }

    void copyToolTip(NOTIFYICONDATAW& data) const
    {
        const int room = sizeof(data.szTip) / sizeof(wchar_t) - 1;
        const QString trimmed = toolTip_.left(room);
        std::memcpy(data.szTip, trimmed.utf16(),
            static_cast<std::size_t>(trimmed.size()) * sizeof(wchar_t));
        data.szTip[static_cast<std::size_t>(trimmed.size())] = L'\0';
    }

    bool add() const
    {
        NOTIFYICONDATAW data{};
        fillIdentification(data);
        data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        data.uCallbackMessage = kCallbackMessage;
        data.hIcon = icon_;
        copyToolTip(data);
        return Shell_NotifyIconW(NIM_ADD, &data) != FALSE;
    }

    bool remove() const
    {
        NOTIFYICONDATAW data{};
        fillIdentification(data);
        return Shell_NotifyIconW(NIM_DELETE, &data) != FALSE;
    }

    void handleMessage(UINT message, WPARAM, LPARAM lParam)
    {
        if (message == taskbarCreatedMessage()) {
            if (visible_) {
                add(); // Explorer restarted; the registration was lost.
            }
            return;
        }
        if (message != kCallbackMessage) {
            return;
        }
        // Legacy (version 0) contract: lParam carries the mouse message.
        switch (LOWORD(lParam)) {
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case NIN_BALLOONUSERCLICK:
            emit owner_->activated();
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            emit owner_->contextMenuRequested(QCursor::pos());
            break;
        default:
            break;
        }
    }

    [[nodiscard]] bool visible() const { return visible_; }
    void setVisible(bool visible) { visible_ = visible; }
    [[nodiscard]] HWND window() const { return window_; }
    [[nodiscard]] QString toolTip() const { return toolTip_; }
    void setToolTip(const QString& text) { toolTip_ = text; }

private:
    static LRESULT CALLBACK windowProc(
        HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(window, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        }
        const auto* self = reinterpret_cast<Impl*>(
            GetWindowLongPtrW(window, GWLP_USERDATA));
        if (self != nullptr && message != WM_NCDESTROY) {
            const_cast<Impl*>(self)->handleMessage(message, wParam, lParam);
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    NativeTrayIcon* owner_ = nullptr;
    HWND window_ = nullptr;
    HICON icon_ = nullptr;
    QString toolTip_;
    bool visible_ = false;
};

NativeTrayIcon::NativeTrayIcon(const QIcon& icon, QObject* parent)
    : QObject(parent)
    , impl_(new Impl(this, icon))
{
}

NativeTrayIcon::~NativeTrayIcon()
{
    delete impl_;
}

bool NativeTrayIcon::show()
{
    if (impl_->visible()) {
        return true;
    }
    if (!impl_->ensureWindow()) {
        return false;
    }
    if (!impl_->add()) {
        return false;
    }
    impl_->setVisible(true);
    return true;
}

void NativeTrayIcon::hide()
{
    if (!impl_->visible()) {
        return;
    }
    impl_->remove();
    impl_->setVisible(false);
}

bool NativeTrayIcon::isVisible() const
{
    return impl_->visible();
}

void NativeTrayIcon::setToolTip(const QString& text)
{
    impl_->setToolTip(text);
    if (!impl_->visible()) {
        return;
    }
    NOTIFYICONDATAW data{};
    impl_->fillIdentification(data);
    data.uFlags = NIF_TIP;
    impl_->copyToolTip(data);
    Shell_NotifyIconW(NIM_MODIFY, &data);
}

void NativeTrayIcon::showMessage(const QString& title, const QString& message)
{
    if (!impl_->visible()) {
        return;
    }
    NOTIFYICONDATAW data{};
    impl_->fillIdentification(data);
    data.uFlags = NIF_INFO;
    data.dwInfoFlags = NIIF_INFO | NIIF_RESPECT_QUIET_TIME;
    const int titleRoom = sizeof(data.szInfoTitle) / sizeof(wchar_t) - 1;
    const int infoRoom = sizeof(data.szInfo) / sizeof(wchar_t) - 1;
    const QString trimmedTitle = title.left(titleRoom);
    const QString trimmedInfo = message.left(infoRoom);
    std::memcpy(data.szInfoTitle, trimmedTitle.utf16(),
        static_cast<std::size_t>(trimmedTitle.size()) * sizeof(wchar_t));
    data.szInfoTitle[static_cast<std::size_t>(trimmedTitle.size())] = L'\0';
    std::memcpy(data.szInfo, trimmedInfo.utf16(),
        static_cast<std::size_t>(trimmedInfo.size()) * sizeof(wchar_t));
    data.szInfo[static_cast<std::size_t>(trimmedInfo.size())] = L'\0';
    Shell_NotifyIconW(NIM_MODIFY, &data);
}

void NativeTrayIcon::beginMenuInteraction()
{
    if (impl_->window() != nullptr) {
        SetForegroundWindow(impl_->window());
    }
}

void NativeTrayIcon::endMenuInteraction()
{
    if (impl_->window() != nullptr) {
        PostMessageW(impl_->window(), WM_NULL, 0, 0);
    }
}

} // namespace snipnexs
