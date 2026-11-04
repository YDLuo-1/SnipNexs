#pragma once

#include <QList>
#include <QString>

namespace snipnexs {

struct TranslationModelFile {
    QString fileName;     // name inside the installed package directory
    QString remotePath;   // asset name appended to the distribution base URL
    QByteArray sha256;    // lowercase hex digest of the file contents
    qint64 sizeBytes = 0; // expected size in bytes; 0 = not enforced
};

struct TranslationModelSpec {
    QString id;             // "hy-mt2-1.8b-q4"
    QString sourceLanguage; // "en" or "zh"; "auto" for prompt-driven engines
    QString targetLanguage; // "en" or "zh"; "auto" likewise
    QString licenseNote;    // attribution shown when downloading
    QString distributionBase; // release URL the asset names resolve against
    QList<TranslationModelFile> files;
};

[[nodiscard]] QList<TranslationModelSpec> knownTranslationModels();

// Normalizes "zh-Hans"/"en-US" to "zh"/"en"; empty for an absent or
// unsupported tag.
[[nodiscard]] QString normalizedTranslationLanguage(const QString& languageTag);

// A script-based default for editable OCR text, not the recognizer's language.
// Mixed Chinese/Latin defaults to Chinese; users can override the target.
[[nodiscard]] QString translationSourceLanguage(const QString& text);

// An explicit target wins; otherwise the source tag decides the direction.
// The prompt-driven model defaults unknown sources to Chinese.
[[nodiscard]] QString translationTargetLanguage(
    const QString& sourceLanguageTag,
    const QString& requestedTarget);

// Resolves the Hy-MT2 package for the requested translation direction.
// Returns false when no package covers the request.
[[nodiscard]] bool findTranslationModelSpec(
    const QString& sourceLanguageTag,
    const QString& targetLanguage,
    TranslationModelSpec& spec);

// The release URL a package's assets resolve against. An INI setting override
// (translation/modelBaseUrl) takes precedence for all packages; SHA-256
// verification applies regardless of the source.
[[nodiscard]] QString effectiveModelBaseUrl(const TranslationModelSpec& spec);

[[nodiscard]] QString defaultLlamaModelsBaseUrl();

// models/translation next to the executable when that directory is writable,
// the Windows local app-data directory otherwise (same rule as the capture
// history store).
[[nodiscard]] QString translationModelsRoot();

[[nodiscard]] QString translationModelDirectory(const QString& packageId, const QString& root = {});
[[nodiscard]] QString translationModelLockPath(const QString& packageId, const QString& root = {});

[[nodiscard]] bool isTranslationModelInstalled(const TranslationModelSpec& spec, const QString& root = {});

// Installed state and on-disk size for every catalog entry; installedBytes is
// 0 when the package is absent. packageBytes is the catalog's declared
// download size.
struct TranslationModelStatus {
    QString id;
    bool installed = false;
    qint64 installedBytes = 0;
    qint64 packageBytes = 0;
};

[[nodiscard]] QList<TranslationModelStatus> translationModelStatuses(const QString& root = {});

// Deletes an installed package directory. The id must exist in the catalog,
// so only model directories this application owns can be removed. Returns
// false and fills error otherwise.
bool removeTranslationModel(const QString& packageId, QString* error, const QString& root = {});

// Writes the package manifest atomically; callers must do this only after all
// package files have been verified in place. The manifest marks the package
// as complete for isTranslationModelInstalled().
bool writeTranslationModelManifest(const TranslationModelSpec& spec, const QString& root = {});

} // namespace snipnexs
