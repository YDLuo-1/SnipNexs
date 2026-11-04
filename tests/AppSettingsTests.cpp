#include "app/AppSettings.h"

#include <QCoreApplication>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextStream>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QDir().mkpath(QStringLiteral("test-temp"));
    QTemporaryDir temporary(QDir::current().filePath(QStringLiteral("test-temp/settings-XXXXXX")));
    if (!temporary.isValid()) { return 1; }
    const QDir root(temporary.path());
    const QString portable = root.filePath(QStringLiteral("portable"));
    const QString fallback = root.filePath(QStringLiteral("local/SnipNexs"));
    QDir().mkpath(portable);
    snipnexs::configureAppSettings(portable, fallback);

    bool ok = true;
    auto check = [&](bool condition, const char* name) {
        QTextStream(stdout) << name << ": " << (condition ? "ok" : "failed") << '\n';
        ok &= condition;
    };
    {
        // The application store must remain INI even if another caller changes Qt's default.
        QSettings::setDefaultFormat(QSettings::NativeFormat);
        snipnexs::AppSettings settings;
        check(settings.format() == QSettings::IniFormat, "explicit INI backend");
        check(!settings.fallbacksEnabled(), "no inherited configuration");
        check(settings.fileName() == QDir(portable).filePath(QStringLiteral("SnipNexs.ini")),
            "portable path");
        settings.setValue(QStringLiteral("ui/language"), QStringLiteral("en"));
        settings.setValue(QStringLiteral("translation/modelBaseUrl"), QStringLiteral("file:///D:/模型"));
        settings.sync();
        check(settings.status() == QSettings::NoError && QFileInfo::exists(settings.fileName()),
            "portable file persisted");
    }
    snipnexs::configureAppSettings(portable, fallback);
    {
        snipnexs::AppSettings settings;
        check(settings.value(QStringLiteral("ui/language")) == QStringLiteral("en")
                && settings.value(QStringLiteral("translation/modelBaseUrl")) == QStringLiteral("file:///D:/模型"),
            "existing portable configuration reopened");
    }

    QFile blocked(root.filePath(QStringLiteral("not-a-directory")));
    if (!blocked.open(QIODevice::WriteOnly)) { return 1; }
    blocked.close();
    snipnexs::configureAppSettings(blocked.fileName(), fallback);
    {
        snipnexs::AppSettings settings;
        check(settings.fileName() == QDir(fallback).filePath(QStringLiteral("SnipNexs.ini")),
            "unwritable executable directory uses local file");
        check(!settings.contains(QStringLiteral("ui/language")), "stores are independent");
        settings.setValue(QStringLiteral("recording/audio"), QStringLiteral("microphone"));
        settings.sync();
        check(settings.status() == QSettings::NoError && QFileInfo::exists(settings.fileName()),
            "fallback directory created and persisted");
    }

    const QString collision = root.filePath(QStringLiteral("collision"));
    QDir().mkpath(QDir(collision).filePath(QStringLiteral("SnipNexs.ini")));
    snipnexs::configureAppSettings(collision, fallback);
    {
        snipnexs::AppSettings settings;
        settings.setValue(QStringLiteral("ui/language"), QStringLiteral("en"));
        settings.sync();
        check(settings.fileName() == QDir(collision).filePath(QStringLiteral("SnipNexs.ini"))
                && settings.status() != QSettings::NoError,
            "existing unusable portable store reports failure without switching");
    }
    QTextStream(stdout) << "user-local data root: "
                        << QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) << '\n';
    return ok ? 0 : 1;
}
