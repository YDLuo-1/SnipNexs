#include "app/AboutDialog.h"

#include <QApplication>
#include <QLabel>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextStream>

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
        << " tabs=" << (tabs != nullptr ? tabs->count() : 0)
        << " text-bytes=" << allLicenseText.toUtf8().size() << '\n';
    return ok ? 0 : 1;
}
