#pragma once

#include <QList>
#include <QString>

namespace snipnexs {

// The default engine is fast and small; the high-quality engine trades a
// larger download and slower inference for near-LLM translation quality.
enum class TranslationEngineId {
    OpusMt,     // one small package per language direction
    LlamaHyMT2, // one larger GGUF package covering both zh-en directions
};

struct TranslationModelFile {
    QString fileName;     // name inside the installed package directory
    QString remotePath;   // asset name appended to the distribution base URL
    QByteArray sha256;    // lowercase hex digest of the file contents
    qint64 sizeBytes = 0; // expected size in bytes; 0 = not enforced
};

struct TranslationModelSpec {
    QString id;             // e.g. "opus-mt-en-zh-int8"
    TranslationEngineId engine = TranslationEngineId::OpusMt;
    QString sourceLanguage; // "en" or "zh"; "auto" for prompt-driven engines
    QString targetLanguage; // "en" or "zh"; "auto" likewise
    QString licenseNote;    // attribution shown when downloading
    QString distributionBase; // release URL the asset names resolve against
    QList<TranslationModelFile> files;
};

[[nodiscard]] QList<TranslationModelSpec> knownTranslationModels();

// Resolves a model package for the requested engine. The OCR tag decides
// the direction when the caller did not ask for a specific target.
// Returns false when no package covers the request.
[[nodiscard]] bool findTranslationModelSpec(
    TranslationEngineId engine,
    const QString& sourceLanguageTag,
    const QString& targetLanguage,
    TranslationModelSpec& spec);

// The release URL a package's assets resolve against. A QSettings override
// (translation/modelBaseUrl) takes precedence for all packages; SHA-256
// verification applies regardless of the source.
[[nodiscard]] QString effectiveModelBaseUrl(const TranslationModelSpec& spec);

[[nodiscard]] QString defaultOpusModelsBaseUrl();
[[nodiscard]] QString defaultLlamaModelsBaseUrl();

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
