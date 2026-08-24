#include "app/AboutDialog.h"
#include "app/AppIcon.h"

#include <QApplication>
#include <QColor>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextStream>
#include <QTimer>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    snipnexs::AboutDialog about;
    const auto* product = about.findChild<QLabel*>(QStringLiteral("aboutProduct"));
    bool ok = product != nullptr
        && product->text().contains(QStringLiteral(SNIPNEXS_VERSION))
        && product->text().contains(QStringLiteral("64"));

    QString allAboutText;
    for (const auto* label : about.findChildren<QLabel*>()) {
        allAboutText += label->text();
    }
    ok &= allAboutText.contains(QStringLiteral("GPL-3.0-or-later"));
    ok &= allAboutText.contains(QStringLiteral("github.com/YDLuo-1/SnipNexs"));
    const auto* sourceLink = about.findChild<QLabel*>(QStringLiteral("sourceLink"));
    const QColor aboutBackground = about.palette().color(QPalette::Window);
    const QColor aboutText = about.palette().color(QPalette::WindowText);
    const QColor linkColor = sourceLink == nullptr
        ? QColor()
        : sourceLink->palette().color(QPalette::Link);
    ok &= sourceLink != nullptr
        && aboutText.lightnessF() - aboutBackground.lightnessF() > 0.55
        && linkColor.lightnessF() - aboutBackground.lightnessF() > 0.45;

    const QPalette applicationPalette = app.palette();
    bool aboutQtPaletteOk = false;
    if (auto* aboutQtButton = about.findChild<QPushButton*>(
            QStringLiteral("aboutQtButton"))) {
        QTimer::singleShot(0, [&aboutQtPaletteOk]() {
            auto* messageBox = qobject_cast<QMessageBox*>(
                QApplication::activeModalWidget());
            if (messageBox == nullptr) {
                return;
            }
            const QPalette palette = messageBox->palette();
            const QColor background = palette.color(QPalette::Window);
            aboutQtPaletteOk = palette.color(QPalette::WindowText).lightnessF()
                    - background.lightnessF() > 0.55
                && palette.color(QPalette::Link).lightnessF()
                    - background.lightnessF() > 0.45;
            messageBox->accept();
        });
        aboutQtButton->click();
    }
    ok &= aboutQtPaletteOk && app.palette() == applicationPalette;

    const QIcon appIcon = snipnexs::createAppIcon();
    ok &= !appIcon.isNull()
        && !appIcon.pixmap(QSize(16, 16)).isNull()
        && !appIcon.pixmap(QSize(64, 64)).isNull();

    snipnexs::OpenSourceLicensesDialog licenses;
    const auto* tabs = licenses.findChild<QTabWidget*>(QStringLiteral("licenseTabs"));
    ok &= tabs != nullptr && tabs->count() == 3;

    QString allLicenseText;
    for (const auto* browser : licenses.findChildren<QTextBrowser*>()) {
        allLicenseText += browser->toPlainText();
    }
    ok &= allLicenseText.contains(QStringLiteral("GNU GENERAL PUBLIC LICENSE"));
    ok &= allLicenseText.contains(QStringLiteral("GNU LESSER GENERAL PUBLIC LICENSE"));
    ok &= allLicenseText.contains(QStringLiteral("Qt 6.11.2"));
    ok &= allLicenseText.contains(QStringLiteral("qtbase-everywhere-src-6.11.2.tar.xz"));
    ok &= allLicenseText.contains(QStringLiteral("Microsoft SimpleRecorder"));

    QTextStream(stdout)
        << "about dialog: " << (ok ? "ok" : "failed")
        << " version=" << SNIPNEXS_VERSION
        << " contrast=" << (aboutText.lightnessF() - aboutBackground.lightnessF())
        << " qt-contrast=" << aboutQtPaletteOk
        << " tabs=" << (tabs != nullptr ? tabs->count() : 0)
        << " text-bytes=" << allLicenseText.toUtf8().size() << '\n';
    return ok ? 0 : 1;
}
