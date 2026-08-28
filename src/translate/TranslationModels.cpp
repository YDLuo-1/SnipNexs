#include "TranslationModels.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTemporaryFile>

namespace snipnexs {

namespace {

constexpr int kManifestFormat = 1;

// The catalog is static: a package becomes available for download only once
// its exact files and digests are pinned here. sha256 values cover the
// converted CTranslate2 int8 models published as release assets; the
// conversion pipeline is documented in docs/local-translation-decision.md.
TranslationModelSpec createEnZhModel()
{
    TranslationModelSpec spec;
    spec.id = QStringLiteral("opus-mt-en-zh-int8");
    spec.sourceLanguage = QStringLiteral("en");
    spec.targetLanguage = QStringLiteral("zh");
    spec.licenseNote = QStringLiteral(
        "Helsinki-NLP/opus-mt-en-zh (Apache-2.0), converted to CTranslate2 int8");

    spec.files = {
        { QStringLiteral("config.json"),
          QStringLiteral("opus-mt-en-zh-int8/config.json"),
          QByteArrayLiteral("72901fbd8abd89fb5cf4a388f26fc681f5c4c58a1e1a88b30b879f107270e7ee"),
          233 },
        { QStringLiteral("model.bin"),
          QStringLiteral("opus-mt-en-zh-int8/model.bin"),
          QByteArrayLiteral("327584c20bb83c7e89d595bcfa30b6ef3771c10816f707e892c4bbb1f808a8fb"),
          79567635 },
        { QStringLiteral("shared_vocabulary.json"),
          QStringLiteral("opus-mt-en-zh-int8/shared_vocabulary.json"),
          QByteArrayLiteral("4821251fdc0a6c9e889837b42427a90cb3240f71aef89681a5a6de525182a634"),
          1368888 },
        { QStringLiteral("source.spm"),
          QStringLiteral("opus-mt-en-zh-int8/source.spm"),
          QByteArrayLiteral("5775ddc9e3ff2fae91554da56468ad35ff56edaba870fea74447bc7234bfdaa8"),
          806435 },
        { QStringLiteral("target.spm"),
          QStringLiteral("opus-mt-en-zh-int8/target.spm"),
          QByteArrayLiteral("81dc94efa84e4025ef38d25d5d07429fe41e3eb29d44003f1db6fe98487b0052"),
          804600 },
    };
    return spec;
}

TranslationModelSpec createZhEnModel()
{
    TranslationModelSpec spec;
    spec.id = QStringLiteral("opus-mt-zh-en-int8");
    spec.sourceLanguage = QStringLiteral("zh");
    spec.targetLanguage = QStringLiteral("en");
    spec.licenseNote = QStringLiteral(
        "Helsinki-NLP/opus-mt-zh-en (CC-BY 4.0), converted to CTranslate2 int8");

    spec.files = {
        { QStringLiteral("config.json"),
          QStringLiteral("opus-mt-zh-en-int8/config.json"),
          QByteArrayLiteral("72901fbd8abd89fb5cf4a388f26fc681f5c4c58a1e1a88b30b879f107270e7ee"),
          233 },
        { QStringLiteral("model.bin"),
          QStringLiteral("opus-mt-zh-en-int8/model.bin"),
          QByteArrayLiteral("e4955858cae9542bef37424a9b79720e3db2f32501fe62264c0cd3eac6319777"),
          79567635 },
        { QStringLiteral("shared_vocabulary.json"),
          QStringLiteral("opus-mt-zh-en-int8/shared_vocabulary.json"),
          QByteArrayLiteral("55d071d6c63a2dab993f00e77077eca76573ac6964990e2e80de7462344401fb"),
          1368999 },
        { QStringLiteral("source.spm"),
          QStringLiteral("opus-mt-zh-en-int8/source.spm"),
          QByteArrayLiteral("e27a3a1b539f4959ec72ea60e453f49156289f95d4e6000b29332efc45616203"),
          804677 },
        { QStringLiteral("target.spm"),
          QStringLiteral("opus-mt-zh-en-int8/target.spm"),
          QByteArrayLiteral("6a881f4717cd7265f53fea54fd3dc689c767c05338fac7a4590f3088cb2d7855"),
          806530 },
    };
    return spec;
}

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

QList<TranslationModelSpec> knownTranslationModels()
{
    static const QList<TranslationModelSpec> models = {
        createEnZhModel(),
        createZhEnModel(),
    };
    return models;
}

bool findTranslationModelSpec(
    const QString& sourceLanguageTag,
    const QString& targetLanguage,
    TranslationModelSpec& spec)
{
    const QString normalizedTarget = targetLanguage.startsWith(
        QStringLiteral("zh"), Qt::CaseInsensitive)
        ? QStringLiteral("zh")
        : QStringLiteral("en");

    for (const TranslationModelSpec& candidate : knownTranslationModels()) {
        if (candidate.targetLanguage == normalizedTarget
            && candidate.sourceLanguage
                == (normalizedTarget == QStringLiteral("zh")
                    ? QStringLiteral("en")
                    : QStringLiteral("zh"))) {
            spec = candidate;
            return true;
        }
    }

    // The OCR tag decides the direction when the caller did not ask for a
    // specific target: Chinese text goes to English, the rest to Chinese.
    const QString source = sourceLanguageTag.startsWith(
        QStringLiteral("zh"), Qt::CaseInsensitive)
        ? QStringLiteral("zh")
        : QStringLiteral("en");
    for (const TranslationModelSpec& candidate : knownTranslationModels()) {
        if (candidate.sourceLanguage == source
            && candidate.targetLanguage
                == (source == QStringLiteral("zh") ? QStringLiteral("en")
                                                   : QStringLiteral("zh"))) {
            spec = candidate;
            return true;
        }
    }
    return false;
}

QString translationModelsBaseUrl()
{
    return QStringLiteral(
        "https://github.com/YDLuo-1/SnipNexs/releases/download/translation-models-v1");
}

QString translationModelsRoot()
{
    static const QString root = defaultModelsDirectory();
    return root;
}

QString translationModelDirectory(const QString& packageId)
{
    return QDir(translationModelsRoot()).filePath(packageId);
}

bool isTranslationModelInstalled(const TranslationModelSpec& spec)
{
    const QDir packageDir = translationModelDirectory(spec.id);
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
        if (!verified
            || !QFileInfo::exists(packageDir.filePath(fileSpec.fileName))) {
            return false;
        }
    }
    return true;
}

bool writeTranslationModelManifest(const TranslationModelSpec& spec)
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

    QSaveFile file(QDir(translationModelDirectory(spec.id)).filePath(
        QStringLiteral("manifest.json")));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(manifest).toJson(QJsonDocument::Compact));
    return file.commit();
}

} // namespace snipnexs
