#include "TranslationService.h"

#include "LlamaTranslation.h"
#include "TranslationModels.h"
#include "TranslationTextSplitter.h"

#include <QDir>
#include <QElapsedTimer>
#include <QMetaObject>

#include <utility>

namespace snipnexs {

namespace {

// Preserve sentence context; LlamaTranslation also enforces token limits.
constexpr int kLlamaSegmentCharacters = 600;

QString modelFilePathFor(const TranslationModelSpec& spec)
{
    return QDir(translationModelDirectory(spec.id))
        .filePath(spec.files.first().fileName);
}

} // namespace

TranslationService::TranslationService(QObject* parent)
    : QObject(parent)
{
    workerContext_ = new QObject();
    workerContext_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, workerContext_, &QObject::deleteLater);
    workerThread_.setObjectName(QStringLiteral("SnipNexs Translation"));
    workerThread_.start();
}

TranslationService::~TranslationService()
{
    cancel();
    workerThread_.quit();
    workerThread_.wait();
    if (llamaSession_) {
        local_llama::closeSession(
            static_cast<local_llama::Session*>(llamaSession_));
    }
}

void TranslationService::cancel()
{
    cancelRequested_.store(true);
}

bool TranslationService::translate(
    const QString& text, const QString& sourceLanguageTag,
    const QString& targetLanguage)
{
    return startTranslation(
        splitTranslationSegments(text, kLlamaSegmentCharacters),
        false, sourceLanguageTag, targetLanguage);
}

bool TranslationService::translateLines(
    const QStringList& lines, const QString& sourceLanguageTag,
    const QString& targetLanguage)
{
    return startTranslation(lines, true, sourceLanguageTag, targetLanguage);
}

bool TranslationService::startTranslation(
    const QStringList& input, bool aligned, const QString& sourceLanguageTag,
    const QString& targetLanguage)
{
    if (busy_ || input.join(QString()).trimmed().isEmpty()) {
        return false;
    }
    const QString target = translationTargetLanguage(sourceLanguageTag, targetLanguage);
    TranslationModelSpec spec;
    if (target.isEmpty() || !findTranslationModelSpec(sourceLanguageTag, target, spec)) {
        emit failed(tr("当前语言组合暂无本地翻译模型。"));
        return false;
    }
    if (!isTranslationModelInstalled(spec)) {
        emit modelMissing(spec.id, spec.licenseNote);
        return false;
    }

    cancelRequested_.store(false);
    busy_ = true;
    emit busyChanged(true);
    QMetaObject::invokeMethod(workerContext_,
        [this, input, aligned, spec = std::move(spec), target]() {
            QElapsedTimer timer;
            timer.start();
            QString error;
            QString text;
            QStringList lines;
            if (!cancelRequested_.load()) {
                if (!llamaSession_ || llamaPackageId_ != spec.id) {
                    local_llama::closeSession(static_cast<local_llama::Session*>(llamaSession_));
                    llamaSession_ = local_llama::openSession(modelFilePathFor(spec), &error, &cancelRequested_);
                    llamaPackageId_ = llamaSession_ ? spec.id : QString();
                }
                if (llamaSession_ && !cancelRequested_.load()) {
                    auto& session = *static_cast<local_llama::Session*>(llamaSession_);
                    if (aligned) {
                        lines = local_llama::translateLines(session, input, target, &error, &cancelRequested_);
                    } else {
                        text = local_llama::translateSegments(session, input, target, &error, &cancelRequested_);
                    }
                }
            }
            const qint64 elapsedMs = timer.elapsed();
            QMetaObject::invokeMethod(this,
                [this, aligned, text = std::move(text), lines = std::move(lines),
                 error = std::move(error), elapsedMs]() {
                    const bool wasCanceled = cancelRequested_.load();
                    busy_ = false;
                    if (wasCanceled) {
                        emit canceled();
                    } else if (!error.isEmpty()) {
                        emit failed(error);
                    } else if (aligned) {
                        emit linesTranslated(lines, elapsedMs);
                    } else {
                        emit translated(text, elapsedMs);
                    }
                    emit busyChanged(busy_);
                }, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    return true;
}

} // namespace snipnexs
