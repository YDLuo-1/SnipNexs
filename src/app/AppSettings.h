#pragma once

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryFile>

namespace snipnexs {

inline void configureAppSettings(
    const QString& portableDirectory, const QString& fallbackDirectory)
{
    QTemporaryFile probe(QDir(portableDirectory).filePath(QStringLiteral(".settings-write-XXXXXX")));
    // Keep an existing portable file authoritative even when it becomes read-only.
    const bool portable = QFileInfo::exists(
        QDir(portableDirectory).filePath(QStringLiteral("SnipNexs.ini"))) || probe.open();
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
        portable ? portableDirectory : fallbackDirectory);
}

class AppSettings final : public QSettings
{
public:
    AppSettings()
        : QSettings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("SnipNexs"))
    {
        setFallbacksEnabled(false);
    }
};

} // namespace snipnexs
