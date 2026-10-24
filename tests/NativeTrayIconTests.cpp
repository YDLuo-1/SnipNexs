#include "platform/windows/NativeTrayIcon.h"
#include "app/AppIcon.h"

#include <QApplication>
#include <QSignalSpy>
#include <QTextStream>
#include <QTimer>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    snipnexs::NativeTrayIcon tray(snipnexs::createAppIcon());
    tray.setToolTip(QStringLiteral("SnipNexs tray test"));

    if (!tray.show()) {
        // Sessions without a shell tray (for example a service session) are
        // not a failure of the wrapper.
        QTextStream(stdout) << "native tray icon: skipped (shell rejected registration)\n";
        return 77;
    }

    bool ok = tray.isVisible();

    QSignalSpy activatedSpy(&tray, &snipnexs::NativeTrayIcon::activated);
    QSignalSpy menuSpy(&tray, &snipnexs::NativeTrayIcon::contextMenuRequested);
    tray.setToolTip(QStringLiteral("SnipNexs tray test updated"));
    tray.showMessage(
        QStringLiteral("SnipNexs tray test"), QStringLiteral("balloon probe"));
    tray.hide();
    ok &= !tray.isVisible();
    ok &= activatedSpy.isEmpty() && menuSpy.isEmpty();

    ok &= tray.show() && tray.isVisible();
    tray.hide();

    QTextStream(stdout) << "native tray icon: " << (ok ? "ok" : "failed") << '\n';
    return ok ? 0 : 1;
}
