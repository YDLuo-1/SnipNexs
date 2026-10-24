#include "TranslationService.h"

#include "LlamaTranslation.h"
#include "LocalTranslation.h"
#include "TranslationModels.h"
#include "TranslationTextSplitter.h"

#include <QDir>
#include <QElapsedTimer>
#include <QMetaObject>

#include <utility>

namespace snipnexs {

namespace {

struct SessionSlot {
    void*& session;
    QString& packageId;
};

// Hy-MT2 has a much larger context than OPUS-MT, so fewer, longer segments
// preserve more sentence context while still fitting the 2048-token window.
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
    workerThread_.quit();
    workerThread_.wait();
    if (opusSession_) {
        local_translation::closeSession(
            static_cast<local_translation::Session*>(opusSession_));
    }
    if (llamaSession_) {
        local_llama::closeSession(
            static_cast<local_llama::Session*>(llamaSession_));
    }
}

bool TranslationService::translate(
    const QString& text,
    const QString& sourceLanguageTag,
    const QString& targetLanguage,
    TranslationEngineId engine)
{
    if (busy_ || text.trimmed().isEmpty()) {
        return false;
    }

    const QString target = translationTargetLanguage(
        engine, sourceLanguageTag, targetLanguage);
    TranslationModelSpec spec;
    if (target.isEmpty()
        || !findTranslationModelSpec(engine, sourceLanguageTag, target, spec)) {
        emit failed(tr("当前语言组合暂无本地翻译模型。"));
        return false;
    }
    if (!isTranslationModelInstalled(spec)) {
        emit modelMissing(spec.id, spec.licenseNote);
        return false;
    }

    busy_ = true;
    emit busyChanged(true);
    QMetaObject::invokeMethod(
        workerContext_,
        [this, text = text, spec = std::move(spec), engine, target]() {
            QElapsedTimer timer;
            timer.start();

            QString error;
            QString resultText;
            if (engine == TranslationEngineId::LlamaHyMT2) {
                if (!llamaSession_ || llamaPackageId_ != spec.id) {
                    if (llamaSession_) {
                        local_llama::closeSession(
                            static_cast<local_llama::Session*>(llamaSession_));
                    }
                    llamaSession_ = local_llama::openSession(
                        modelFilePathFor(spec), &error);
                    llamaPackageId_ = llamaSession_ ? spec.id : QString();
                }
                if (llamaSession_) {
                    resultText = local_llama::translateSegments(
                        *static_cast<local_llama::Session*>(llamaSession_),
                        splitTranslationSegments(text, kLlamaSegmentCharacters),
                        target,
                        &error);
                }
            } else {
                if (!opusSession_ || opusPackageId_ != spec.id) {
                    if (opusSession_) {
                        local_translation::closeSession(
                            static_cast<local_translation::Session*>(opusSession_));
                    }
                    opusSession_ = local_translation::openSession(
                        translationModelDirectory(spec.id), &error);
                    opusPackageId_ = opusSession_ ? spec.id : QString();
                }
                if (opusSession_) {
                    resultText = local_translation::translateSegments(
                        *static_cast<local_translation::Session*>(opusSession_),
                        splitTranslationSegments(text),
                        &error);
                }
            }

            const qint64 elapsedMs = timer.elapsed();
            QMetaObject::invokeMethod(
                this,
                [this, resultText = std::move(resultText),
                 error = std::move(error), elapsedMs]() {
                    busy_ = false;
                    emit busyChanged(false);
                    if (!error.isEmpty()) {
                        emit failed(error);
                    } else {
                        emit translated(resultText, elapsedMs);
                    }
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
    return true;
}

bool TranslationService::translateLines(
    const QStringList& lines,
    const QString& sourceLanguageTag,
    TranslationEngineId engine)
{
    if (busy_ || lines.isEmpty()) {
        return false;
    }
    bool anyContent = false;
    for (const QString& line : lines) {
        if (!line.trimmed().isEmpty()) {
            anyContent = true;
            break;
        }
    }
    if (!anyContent) {
        return false;
    }

    const QString target = translationTargetLanguage(
        engine, sourceLanguageTag, QString());
    TranslationModelSpec spec;
    if (target.isEmpty()
        || !findTranslationModelSpec(engine, sourceLanguageTag, target, spec)) {
        emit failed(tr("当前语言组合暂无本地翻译模型。"));
        return false;
    }
    if (!isTranslationModelInstalled(spec)) {
        emit modelMissing(spec.id, spec.licenseNote);
        return false;
    }

    busy_ = true;
    emit busyChanged(true);
    QMetaObject::invokeMethod(
        workerContext_,
        [this, lines = lines, spec = std::move(spec), engine, target]() {
            QElapsedTimer timer;
            timer.start();

            QString error;
            QStringList resultLines;
            if (engine == TranslationEngineId::LlamaHyMT2) {
                if (!llamaSession_ || llamaPackageId_ != spec.id) {
                    if (llamaSession_) {
                        local_llama::closeSession(
                            static_cast<local_llama::Session*>(llamaSession_));
                    }
                    llamaSession_ = local_llama::openSession(
                        modelFilePathFor(spec), &error);
                    llamaPackageId_ = llamaSession_ ? spec.id : QString();
                }
                if (llamaSession_) {
                    resultLines = local_llama::translateLines(
                        *static_cast<local_llama::Session*>(llamaSession_),
                        lines, target, &error);
                }
            } else {
                if (!opusSession_ || opusPackageId_ != spec.id) {
                    if (opusSession_) {
                        local_translation::closeSession(
                            static_cast<local_translation::Session*>(opusSession_));
                    }
                    opusSession_ = local_translation::openSession(
                        translationModelDirectory(spec.id), &error);
                    opusPackageId_ = opusSession_ ? spec.id : QString();
                }
                if (opusSession_) {
                    resultLines = local_translation::translateLines(
                        *static_cast<local_translation::Session*>(opusSession_),
                        lines, &error);
                }
            }

            const qint64 elapsedMs = timer.elapsed();
            QMetaObject::invokeMethod(
                this,
                [this, resultLines = std::move(resultLines),
                 error = std::move(error), elapsedMs]() {
                    busy_ = false;
                    emit busyChanged(false);
                    if (!error.isEmpty()) {
                        emit failed(error);
                    } else {
                        emit linesTranslated(resultLines, elapsedMs);
                    }
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
    return true;
}

} // namespace snipnexs
