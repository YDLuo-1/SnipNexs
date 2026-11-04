#include "app/MainWindow.h"
#include <QApplication>
#include <QDir>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextStream>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QDir().mkpath(QStringLiteral("test-temp"));
    QTemporaryDir temporary(QDir::current().filePath(QStringLiteral("test-temp/main-ui-XXXXXX")));
    if (!temporary.isValid()) { return 1; }
    QCoreApplication::setOrganizationName(QStringLiteral("SnipNexsTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
    snipnexs::MainWindow window;
    window.show();
    auto* minimize = window.findChild<QPushButton*>("secondaryButton");
    bool ok = minimize && !window.trayAvailable();
    if (!ok) { return 1; }
    minimize->click();
    app.processEvents();
    ok &= window.isVisible() && window.isMinimized();
    window.showAndActivate();
    ok &= window.isVisible() && !window.isMinimized();
    window.setCaptureActive(true);
    window.showAndActivate();
    ok &= !window.isVisible();
    window.setCaptureActive(false);
    window.showAndActivate();
    ok &= window.isVisible();
    QTextStream(stdout) << "main window fallback: " << (ok ? "ok" : "failed") << '\n';
    return ok ? 0 : 1;
}
