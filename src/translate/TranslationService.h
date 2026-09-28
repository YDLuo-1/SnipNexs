#pragma once

#include <QObject>
#include <QThread>

#include "translate/TranslationModels.h"

class QString;

namespace snipnexs {

// Translates OCR text with the local (offline) engines. Same threading model
// as OcrService: one worker thread, at most one job at a time. Sessions are
// opened lazily on the worker thread and reused across calls, one slot per
// engine so switching engines does not reload models.
class TranslationService final : public QObject
{
    Q_OBJECT

public:
    explicit TranslationService(QObject* parent = nullptr);
    ~TranslationService() override;

    // Returns true when the job was accepted; translated() or failed() will
    // be emitted later. Returns false when busy or when no model exists for
    // the request; in the latter case modelMissing() is emitted so the
    // caller can offer the download.
    [[nodiscard]] bool translate(
        const QString& text,
        const QString& sourceLanguageTag,
        const QString& targetLanguage,
        TranslationEngineId engine);

signals:
    void translated(const QString& text, qint64 elapsedMs);
    void failed(const QString& message);
    void modelMissing(const QString& packageId, const QString& licenseNote);
    void busyChanged(bool busy);

private:
    QThread workerThread_;
    QObject* workerContext_ = nullptr;
    bool busy_ = false;

    // Worker-thread-only state; erased as void* so third-party headers never
    // leak into this header. One session slot per engine.
    void* opusSession_ = nullptr;
    QString opusPackageId_;
    void* llamaSession_ = nullptr;
    QString llamaPackageId_;
};

} // namespace snipnexs
