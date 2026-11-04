#pragma once

#include <QObject>
#include <QStringList>
#include <QThread>
#include <atomic>

#include "translate/TranslationModels.h"

class QString;

namespace snipnexs {

// Translates OCR text locally with Hy-MT2. One worker thread, at most one job
// at a time; the model session is opened lazily and reused across calls.
class TranslationService final : public QObject
{
    Q_OBJECT

public:
    explicit TranslationService(QObject* parent = nullptr);
    ~TranslationService() override;
    [[nodiscard]] bool isBusy() const { return busy_; }
    void cancel();

    // Returns true when the job was accepted; translated() or failed() will
    // be emitted later. Returns false when busy or when no model exists for
    // the request; in the latter case modelMissing() is emitted so the
    // caller can offer the download.
    [[nodiscard]] bool translate(
        const QString& text,
        const QString& sourceLanguageTag,
        const QString& targetLanguage);

    // Line-aligned variant for the bilingual OCR view: translates every line
    // independently; linesTranslated() returns exactly one entry per input
    // line. Same busy/model handling as translate().
    [[nodiscard]] bool translateLines(
        const QStringList& lines,
        const QString& sourceLanguageTag,
        const QString& targetLanguage);

signals:
    void translated(const QString& text, qint64 elapsedMs);
    void linesTranslated(const QStringList& lines, qint64 elapsedMs);
    void failed(const QString& message);
    void canceled();
    void modelMissing(const QString& packageId, const QString& licenseNote);
    void busyChanged(bool busy);

private:
    bool startTranslation(const QStringList& input, bool aligned,
        const QString& sourceLanguageTag, const QString& targetLanguage);
    QThread workerThread_;
    QObject* workerContext_ = nullptr;
    bool busy_ = false;
    std::atomic_bool cancelRequested_{false};

    // Worker-thread-only state; erased as void* so third-party headers never
    // leak into this header.
    void* llamaSession_ = nullptr;
    QString llamaPackageId_;
};

} // namespace snipnexs
