#pragma once

#include <QList>
#include <QString>

namespace snipnexs {

struct TranslationModelFile {
    QString fileName;     // name inside the installed package directory
    QString remotePath;   // path appended to the distribution base URL
    QByteArray sha256;    // lowercase hex digest of the file contents
    qint64 sizeBytes = 0; // expected size in bytes; 0 = not enforced
};

struct TranslationModelSpec {
    QString id;             // e.g. "opus-mt-en-zh-int8"
    QString sourceLanguage; // "en" or "zh"
    QString targetLanguage; // "en" or "zh"
    QString licenseNote;    // attribution shown when downloading
    QList<TranslationModelFile> files;
};

[[nodiscard]] QList<TranslationModelSpec> knownTranslationModels();

// Chinese text uses the zh->en package, anything else en->zh. Returns false
// when the requested direction has no local model.
[[nodiscard]] bool findTranslationModelSpec(
    const QString& sourceLanguageTag,
    const QString& targetLanguage,
    TranslationModelSpec& spec);

[[nodiscard]] QString translationModelsBaseUrl();

// models/translation next to the executable when that directory is writable,
// the Windows local app-data directory otherwise (same rule as the capture
// history store).
[[nodiscard]] QString translationModelsRoot();

[[nodiscard]] QString translationModelDirectory(const QString& packageId);

[[nodiscard]] bool isTranslationModelInstalled(const TranslationModelSpec& spec);

// Writes the package manifest atomically; callers must do this only after all
// package files have been verified in place. The manifest marks the package
// as complete for isTranslationModelInstalled().
bool writeTranslationModelManifest(const TranslationModelSpec& spec);

} // namespace snipnexs
