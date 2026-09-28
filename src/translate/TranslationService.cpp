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

    TranslationModelSpec spec;
    if (!findTranslationModelSpec(engine, sourceLanguageTag, targetLanguage, spec)) {
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
        [this, text = text, spec = std::move(spec), engine, targetLanguage]() {
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
                        splitTranslationSegments(text),
                        targetLanguage.startsWith(QStringLiteral("zh"),
                                                  Qt::CaseInsensitive)
                            ? QStringLiteral("zh")
                            : QStringLiteral("en"),
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

} // namespace snipnexs
