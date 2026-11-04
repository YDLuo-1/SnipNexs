#include "TranslationModels.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryFile>

namespace snipnexs {

namespace {

constexpr int kManifestFormat = 1;

QString defaultModelsDirectory()
{
    const QString applicationDirectory = QCoreApplication::applicationDirPath();
    if (!applicationDirectory.isEmpty()) {
        const QString applicationModels = QDir(applicationDirectory).filePath(
            QStringLiteral("models/translation"));
        if (QDir().mkpath(applicationModels)) {
            QTemporaryFile probe(QDir(applicationModels).filePath(
                QStringLiteral(".write-test-XXXXXX")));
            if (probe.open()) {
                probe.close();
                probe.remove();
                return applicationModels;
            }
        }
    }

    QString base = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty()) {
        base = QDir::homePath();
    }
    return QDir(base).filePath(QStringLiteral("models/translation"));
}

} // namespace

TranslationModelSpec createHyMt2Model()
{
    // Upstream's 2-bit and 1.25-bit GGUFs need unmerged llama.cpp kernels
    // (PR #19357/#22836); Q4_K_M is the smallest package the pinned build
    // can run reproducibly.
    TranslationModelSpec spec;
    spec.id = QStringLiteral("hy-mt2-1.8b-q4");
    spec.distributionBase = defaultLlamaModelsBaseUrl();
    spec.sourceLanguage = QStringLiteral("auto");
    spec.targetLanguage = QStringLiteral("auto");
    spec.licenseNote = QStringLiteral(
        "tencent/Hy-MT2-1.8B GGUF Q4_K_M (Apache-2.0), runs via llama.cpp");
    spec.files = {
        { QStringLiteral("Hy-MT2-1.8B-Q4_K_M.gguf"),
          QStringLiteral("Hy-MT2-1.8B-Q4_K_M.gguf"),
          QByteArrayLiteral("dc5f44fcf1fa496ee7ad725982c0c8c553a4de00259b53af84c4b89fb0c06699"),
          1133080448 },
    };
    return spec;
}

QList<TranslationModelSpec> knownTranslationModels()
{
    static const QList<TranslationModelSpec> models = {
        createHyMt2Model(),
    };
    return models;
}

QString normalizedTranslationLanguage(const QString& languageTag)
{
    const QString trimmed = languageTag.trimmed();
    if (trimmed.startsWith(QStringLiteral("zh"), Qt::CaseInsensitive)) {
        return QStringLiteral("zh");
    }
    if (trimmed.startsWith(QStringLiteral("en"), Qt::CaseInsensitive)) {
        return QStringLiteral("en");
    }
    return {};
}

QString translationSourceLanguage(const QString& text)
{
    bool chinese = false;
    bool latin = false;
    for (const char32_t code : text.toUcs4()) {
        const auto script = QChar::script(code);
        if (script == QChar::Script_Han) { chinese = true; }
        else if (script == QChar::Script_Latin) { latin = true; }
        else if (QChar::isLetter(code)) { return {}; }
    }
    return chinese ? QStringLiteral("zh") : latin ? QStringLiteral("en") : QString();
}

QString translationTargetLanguage(
    const QString& sourceLanguageTag,
    const QString& requestedTarget)
{
    const QString requested = normalizedTranslationLanguage(requestedTarget);
    if (!requested.isEmpty()) {
        return requested;
    }
    if (!requestedTarget.trimmed().isEmpty()) {
        return {};
    }

    const QString source = normalizedTranslationLanguage(sourceLanguageTag);
    if (source == QStringLiteral("zh")) {
        return QStringLiteral("en");
    }
    if (source == QStringLiteral("en")) {
        return QStringLiteral("zh");
    }
    return QStringLiteral("zh");
}

bool findTranslationModelSpec(
    const QString& sourceLanguageTag,
    const QString& targetLanguage,
    TranslationModelSpec& spec)
{
    const QString source = normalizedTranslationLanguage(sourceLanguageTag);
    const QString target = translationTargetLanguage(sourceLanguageTag, targetLanguage);
    if (target.isEmpty() || (!source.isEmpty() && source == target)) {
        return false;
    }
    spec = knownTranslationModels().first();
    return true;
}

QString defaultLlamaModelsBaseUrl()
{
    return QStringLiteral(
        "https://github.com/YDLuo-1/SnipNexs/releases/download/translation-models-hymt2-v1");
}

QString effectiveModelBaseUrl(const TranslationModelSpec& spec)
{
    // Users behind restrictive networks can point this at a mirror or a
    // local server (http(s)/file URL) hosting the same asset layout;
    // SHA-256 verification applies regardless of the source.
    const QString overrideUrl = QSettings().value(
        QStringLiteral("translation/modelBaseUrl")).toString().trimmed();
    if (!overrideUrl.isEmpty()) {
        return overrideUrl;
    }
    return spec.distributionBase;
}

QString translationModelsRoot()
{
    static const QString root = defaultModelsDirectory();
    return root;
}

QString translationModelDirectory(const QString& packageId, const QString& root)
{
    return QDir(root.isEmpty() ? translationModelsRoot() : root).filePath(packageId);
}

QString translationModelLockPath(const QString& packageId, const QString& root)
{
    return translationModelDirectory(packageId, root) + QStringLiteral(".lock");
}

bool isTranslationModelInstalled(const TranslationModelSpec& spec, const QString& root)
{
    const QDir packageDir = translationModelDirectory(spec.id, root);
    QFile manifestFile(packageDir.filePath(QStringLiteral("manifest.json")));
    if (!manifestFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonObject manifest = QJsonDocument::fromJson(
        manifestFile.readAll()).object();
    if (manifest.value(QStringLiteral("format")).toInt() != kManifestFormat
        || manifest.value(QStringLiteral("id")).toString() != spec.id) {
        return false;
    }

    const QJsonArray entries = manifest.value(QStringLiteral("files")).toArray();
    for (const TranslationModelFile& fileSpec : spec.files) {
        bool verified = false;
        for (const QJsonValue& value : entries) {
            const QJsonObject entry = value.toObject();
            if (entry.value(QStringLiteral("name")).toString() == fileSpec.fileName
                && entry.value(QStringLiteral("sha256")).toString()
                    .compare(QString::fromLatin1(fileSpec.sha256),
                             Qt::CaseInsensitive) == 0) {
                verified = true;
                break;
            }
        }
        const QFileInfo file(packageDir.filePath(fileSpec.fileName));
        if (!verified || !file.isFile()
            || (fileSpec.sizeBytes > 0 && file.size() != fileSpec.sizeBytes)) {
            return false;
        }
    }
    return true;
}

bool writeTranslationModelManifest(const TranslationModelSpec& spec, const QString& root)
{
    QJsonArray files;
    for (const TranslationModelFile& fileSpec : spec.files) {
        files.append(QJsonObject{
            { QStringLiteral("name"), fileSpec.fileName },
            { QStringLiteral("sha256"), QString::fromLatin1(fileSpec.sha256) },
        });
    }

    QJsonObject manifest;
    manifest.insert(QStringLiteral("format"), kManifestFormat);
    manifest.insert(QStringLiteral("id"), spec.id);
    manifest.insert(QStringLiteral("sourceLanguage"), spec.sourceLanguage);
    manifest.insert(QStringLiteral("targetLanguage"), spec.targetLanguage);
    manifest.insert(QStringLiteral("license"), spec.licenseNote);
    manifest.insert(QStringLiteral("files"), files);

    QSaveFile file(QDir(translationModelDirectory(spec.id, root)).filePath(
        QStringLiteral("manifest.json")));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(manifest).toJson(QJsonDocument::Compact));
    return file.commit();
}

namespace {

qint64 directorySizeBytes(const QString& path)
{
    qint64 total = 0;
    QDirIterator iterator(path, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        total += QFileInfo(iterator.next()).size();
    }
    return total;
}

} // namespace

QList<TranslationModelStatus> translationModelStatuses(const QString& root)
{
    QList<TranslationModelStatus> statuses;
    for (const TranslationModelSpec& spec : knownTranslationModels()) {
        TranslationModelStatus status;
        status.id = spec.id;
        status.installed = isTranslationModelInstalled(spec, root);
        status.installedBytes = status.installed
            ? directorySizeBytes(translationModelDirectory(spec.id, root))
            : 0;
        for (const TranslationModelFile& file : spec.files) {
            status.packageBytes += file.sizeBytes;
        }
        statuses.append(status);
    }
    return statuses;
}

bool removeTranslationModel(const QString& packageId, QString* error, const QString& root)
{
    for (const TranslationModelSpec& spec : knownTranslationModels()) {
        if (spec.id != packageId) {
            continue;
        }
        const QString directory = translationModelDirectory(spec.id, root);
        if (!QFileInfo::exists(directory)) {
            return true;
        }
        QLockFile lock(translationModelLockPath(spec.id, root));
        lock.setStaleLockTime(0);
        if (!lock.tryLock(0)) {
            if (error) {
                *error = QCoreApplication::translate(
                    "snipnexs::TranslationModels", "该语言包正在安装，请稍后再删除。");
            }
            return false;
        }
        if (!QDir(directory).removeRecursively()) {
            if (error) {
                *error = QCoreApplication::translate(
                    "snipnexs::TranslationModels", "无法删除模型目录：%1")
                    .arg(QDir::toNativeSeparators(directory));
            }
            return false;
        }
        return true;
    }
    if (error) {
        *error = QCoreApplication::translate(
            "snipnexs::TranslationModels", "未找到该语言包。");
    }
    return false;
}

} // namespace snipnexs
