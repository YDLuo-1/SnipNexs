#include "app/MainWindow.h"
#include "app/AppSettings.h"
#include "app/SettingsDialog.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>

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

    snipnexs::SettingsDialog dialog;
    dialog.setAttribute(Qt::WA_DeleteOnClose, false);
    dialog.findChild<QComboBox*>()->setCurrentIndex(1);
    dialog.findChild<QLineEdit*>()->setText(QStringLiteral("  https://example.invalid/models  "));
    dialog.findChild<QCheckBox*>()->setChecked(true);
    dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    {
        snipnexs::AppSettings saved;
        ok &= dialog.result() == QDialog::Accepted
            && saved.value(QStringLiteral("recording/audio")) == QStringLiteral("microphone")
            && saved.value(QStringLiteral("translation/modelBaseUrl")) == QStringLiteral("https://example.invalid/models")
            && saved.value(QStringLiteral("update/checkAtStartup")).toBool()
            && QFileInfo::exists(saved.fileName());
    }
    QTextStream(stdout) << "settings dialog file persistence: " << (ok ? "ok" : "failed") << '\n';

    const QString collision = QDir(temporary.path()).filePath(QStringLiteral("blocked"));
    QDir().mkpath(QDir(collision).filePath(QStringLiteral("SnipNexs.ini")));
    snipnexs::configureAppSettings(collision, temporary.path());
    snipnexs::SettingsDialog blockedDialog;
    blockedDialog.setAttribute(Qt::WA_DeleteOnClose, false);
    blockedDialog.show();
    bool warningShown = false;
    QTimer::singleShot(0, [&]() {
        auto* warning = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if (warning) {
            warningShown = warning->text().contains(QStringLiteral("SnipNexs.ini"));
            warning->accept();
        }
    });
    blockedDialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    ok &= warningShown && blockedDialog.isVisible() && blockedDialog.result() != QDialog::Accepted;
    blockedDialog.reject();
    QTextStream(stdout) << "settings save failure stays open with warning: " << (ok ? "ok" : "failed") << '\n';
    return ok ? 0 : 1;
}
