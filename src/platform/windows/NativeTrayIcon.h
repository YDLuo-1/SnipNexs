#pragma once

#include <QObject>
#include <QPoint>

#include <QIcon>

class QWidget;

namespace snipnexs {

// Direct Shell_NotifyIcon wrapper. Qt 6.11.2's QSystemTrayIcon registers its
// icon through a code path that Windows 11 build 26300 silently drops (the
// Qt-side message window exists, the icon never appears anywhere in the
// tray), while a plain legacy Shell_NotifyIcon registration on the same
// machine works. This class mirrors the small QSystemTrayIcon surface the
// application uses and keeps the menu hosting in QWidget land.
class NativeTrayIcon final : public QObject
{
    Q_OBJECT

public:
    explicit NativeTrayIcon(const QIcon& icon, QObject* parent = nullptr);
    ~NativeTrayIcon() override;

    // Registers the icon. Returns false when the shell rejects the
    // registration (for example in a session without a tray).
    bool show();
    void hide();
    [[nodiscard]] bool isVisible() const;
    void setToolTip(const QString& text);
    void showMessage(const QString& title, const QString& message);

    // Call around a QMenu::exec() popup opened from contextMenuRequested;
    // the foreground/WM_NULL pair is what lets Windows dismiss the menu when
    // the user clicks elsewhere.
    void beginMenuInteraction();
    void endMenuInteraction();

signals:
    // Left click, double click, or a balloon-notification click.
    void activated();
    void contextMenuRequested(const QPoint& globalPos);

private:
    class Impl;
    Impl* impl_ = nullptr;
};

} // namespace snipnexs
