#pragma once

#include <QObject>
#include <QThread>

class QString;

namespace snipnexs {

// Translates OCR text with the local (offline) engine. Same threading model
// as OcrService: one worker thread, at most one job at a time. The engine is
// opened lazily on the worker thread and reused across calls.
class TranslationService final : public QObject
{
    Q_OBJECT

public:
    explicit TranslationService(QObject* parent = nullptr);
    ~TranslationService() override;

    // Returns true when the job was accepted; translated() or failed() will
    // be emitted later. Returns false when busy or when no model exists for
    // the requested direction; in the latter case modelMissing() is emitted
    // so the caller can offer the download.
    [[nodiscard]] bool translate(
        const QString& text,
        const QString& sourceLanguageTag,
        const QString& targetLanguage);

signals:
    void translated(const QString& text, qint64 elapsedMs);
    void failed(const QString& message);
    void modelMissing(const QString& packageId, const QString& licenseNote);
    void busyChanged(bool busy);

private:
    QThread workerThread_;
    QObject* workerContext_ = nullptr;
    bool busy_ = false;

    // Worker-thread-only state, erased here so CTranslate2 types never leak
    // into this header.
    void* session_ = nullptr;
    QString sessionPackageId_;
};

} // namespace snipnexs
