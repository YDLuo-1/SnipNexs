#include "CaptureHistoryStore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QUuid>

#include <algorithm>
#include <utility>

namespace snipnexs {

namespace {

constexpr int kMaximumEntries = 20;
constexpr qsizetype kMaximumBytes = 64 * 1024 * 1024;
constexpr int kIndexVersion = 1;

struct Entry final
{
    QString fileName;
    QString createdUtc;
    qreal devicePixelRatio = 1.0;
    qsizetype bytes = 0;
};

QString defaultHistoryDirectory()
{
    QString base = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty()) {
        base = QDir::homePath();
    }
    return QDir(base).filePath(QStringLiteral("history"));
}

QString indexPath(const QString& directory)
{
    return QDir(directory).filePath(QStringLiteral("index.json"));
}

bool isSafeFileName(const QString& fileName)
{
    return !fileName.isEmpty()
        && QFileInfo(fileName).fileName() == fileName
        && fileName.startsWith(QStringLiteral("capture-"))
        && fileName.endsWith(QStringLiteral(".png"))
        && !fileName.contains(QStringLiteral(".."));
}

bool readIndex(const QString& directory, QList<Entry>& entries)
{
    entries.clear();
    QFile file(indexPath(directory));
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("version")).toInt() != kIndexVersion
        || !root.value(QStringLiteral("entries")).isArray()) {
        return false;
    }

    for (const QJsonValue& value : root.value(QStringLiteral("entries")).toArray()) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        Entry entry;
        entry.fileName = object.value(QStringLiteral("file")).toString();
        entry.createdUtc = object.value(QStringLiteral("createdUtc")).toString();
        entry.devicePixelRatio = object.value(QStringLiteral("dpr")).toDouble(1.0);
        entry.bytes = static_cast<qsizetype>(
            object.value(QStringLiteral("bytes")).toVariant().toLongLong());
        if (isSafeFileName(entry.fileName)
            && entry.devicePixelRatio > 0.0
            && entry.devicePixelRatio <= 16.0) {
            entries.append(entry);
        }
    }
    return true;
}

bool writeIndex(const QString& directory, const QList<Entry>& entries)
{
    if (!QDir().mkpath(directory)) {
        return false;
    }

    QJsonArray array;
    for (const Entry& entry : entries) {
        QJsonObject object;
        object.insert(QStringLiteral("file"), entry.fileName);
        object.insert(QStringLiteral("createdUtc"), entry.createdUtc);
        object.insert(QStringLiteral("dpr"), entry.devicePixelRatio);
        object.insert(QStringLiteral("bytes"), static_cast<qint64>(entry.bytes));
        array.append(object);
    }
    QJsonObject root;
    root.insert(QStringLiteral("version"), kIndexVersion);
    root.insert(QStringLiteral("entries"), array);

    QSaveFile file(indexPath(directory));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

void removeOrphans(const QString& directory, const QList<Entry>& entries)
{
    QSet<QString> referenced;
    for (const Entry& entry : entries) {
        referenced.insert(entry.fileName);
    }
    const QFileInfoList files = QDir(directory).entryInfoList(
        {QStringLiteral("capture-*.png")}, QDir::Files);
    for (const QFileInfo& file : files) {
        if (!referenced.contains(file.fileName())) {
            QFile::remove(file.absoluteFilePath());
        }
    }
}

bool prune(QList<Entry>& entries)
{
    bool changed = false;
    qsizetype bytes = 0;
    for (const Entry& entry : entries) {
        bytes += std::max<qsizetype>(0, entry.bytes);
    }
    while (entries.size() > 1
        && (entries.size() > kMaximumEntries || bytes > kMaximumBytes)) {
        bytes -= std::max<qsizetype>(0, entries.first().bytes);
        entries.removeFirst();
        changed = true;
    }
    return changed;
}

QList<Entry> validEntries(
    const QString& directory, bool& changed, bool& indexExists)
{
    QList<Entry> indexed;
    const bool validIndex = readIndex(directory, indexed);
    indexExists = QFileInfo::exists(indexPath(directory));
    if (!validIndex) {
        changed = indexExists;
        indexed.clear();
        return indexed;
    }

    QList<Entry> result;
    for (Entry entry : indexed) {
        QImage image(QDir(directory).filePath(entry.fileName));
        if (image.isNull()) {
            changed = true;
            continue;
        }
        const qreal dpr = entry.devicePixelRatio > 0.0
            ? entry.devicePixelRatio
            : 1.0;
        if (entry.bytes != image.sizeInBytes()) {
            entry.bytes = image.sizeInBytes();
            changed = true;
        }
        entry.devicePixelRatio = dpr;
        result.append(entry);
    }
    if (result.size() != indexed.size()) {
        changed = true;
    }
    return result;
}

} // namespace

CaptureHistoryStore::CaptureHistoryStore(QString directory)
    : directory_(directory.isEmpty() ? defaultHistoryDirectory() : std::move(directory))
{
}

QList<QImage> CaptureHistoryStore::load()
{
    bool changed = false;
    bool indexExists = false;
    QList<Entry> entries = validEntries(directory_, changed, indexExists);
    changed |= prune(entries);
    bool indexWriteOk = true;
    if (changed || indexExists) {
        if (!entries.isEmpty() || indexExists) {
            indexWriteOk = writeIndex(directory_, entries);
        }
    }
    if (indexWriteOk) {
        removeOrphans(directory_, entries);
    }

    QList<QImage> images;
    for (const Entry& entry : entries) {
        QImage image(QDir(directory_).filePath(entry.fileName));
        if (image.isNull()) {
            continue;
        }
        image.setDevicePixelRatio(entry.devicePixelRatio);
        images.append(std::move(image));
    }
    return images;
}

bool CaptureHistoryStore::append(const QImage& image)
{
    if (image.isNull()) {
        return false;
    }
    if (!QDir().mkpath(directory_)) {
        return false;
    }

    bool changed = false;
    bool indexExists = false;
    QList<Entry> entries = validEntries(directory_, changed, indexExists);
    changed |= prune(entries);

    const QString fileName = QStringLiteral("capture-%1-%2.png")
        .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")))
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString filePath = QDir(directory_).filePath(fileName);
    QSaveFile imageFile(filePath);
    if (!imageFile.open(QIODevice::WriteOnly) || !image.save(&imageFile, "PNG")
        || !imageFile.commit()) {
        imageFile.cancelWriting();
        return false;
    }

    Entry newEntry;
    newEntry.fileName = fileName;
    newEntry.createdUtc = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    newEntry.devicePixelRatio = image.devicePixelRatio() > 0.0
        ? image.devicePixelRatio()
        : 1.0;
    newEntry.bytes = image.sizeInBytes();
    entries.append(newEntry);

    QList<Entry> retained = entries;
    prune(retained);
    if (!writeIndex(directory_, retained)) {
        QFile::remove(filePath);
        return false;
    }

    removeOrphans(directory_, retained);
    Q_UNUSED(changed);
    Q_UNUSED(indexExists);
    return true;
}

} // namespace snipnexs
