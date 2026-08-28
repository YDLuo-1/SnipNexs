#include "TranslationService.h"

#include "LocalTranslation.h"
#include "TranslationModels.h"
#include "TranslationTextSplitter.h"

#include <QElapsedTimer>
#include <QMetaObject>

#include <utility>

namespace snipnexs {

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
    if (session_) {
        local_translation::closeSession(
            static_cast<local_translation::Session*>(session_));
        session_ = nullptr;
    }
}

bool TranslationService::translate(
    const QString& text,
    const QString& sourceLanguageTag,
    const QString& targetLanguage)
{
    if (busy_ || text.trimmed().isEmpty()) {
        return false;
    }

    TranslationModelSpec spec;
    if (!findTranslationModelSpec(sourceLanguageTag, targetLanguage, spec)) {
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
        [this, text = text, spec = std::move(spec)]() {
            QElapsedTimer timer;
            timer.start();

            QString error;
            if (!session_ || sessionPackageId_ != spec.id) {
                if (session_) {
                    local_translation::closeSession(
                        static_cast<local_translation::Session*>(session_));
                }
                session_ = local_translation::openSession(
                    translationModelDirectory(spec.id), &error);
                sessionPackageId_ = session_ ? spec.id : QString();
            }

            QString resultText;
            if (session_) {
                const QStringList segments = splitTranslationSegments(text);
                resultText = local_translation::translateSegments(
                    *static_cast<local_translation::Session*>(session_),
                    segments,
                    &error);
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
