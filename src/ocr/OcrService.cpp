#include "OcrService.h"

#include "WindowsOcr.h"

#include <QImage>
#include <QMetaObject>

#include <utility>

namespace snipnexs {

OcrService::OcrService(QObject* parent)
    : QObject(parent)
{
    workerContext_ = new QObject();
    workerContext_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, workerContext_, &QObject::deleteLater);
    workerThread_.setObjectName(QStringLiteral("SnipNexs OCR"));
    workerThread_.start();
}

OcrService::~OcrService()
{
    workerThread_.quit();
    workerThread_.wait();
}

bool OcrService::recognize(QImage image)
{
    if (busy_ || image.isNull()) {
        return false;
    }

    busy_ = true;
    QMetaObject::invokeMethod(
        workerContext_,
        [this, image = std::move(image)]() {
            windows_ocr::Result result = windows_ocr::recognize(image);
            QMetaObject::invokeMethod(
                this,
                [this, result = std::move(result)]() {
                    busy_ = false;
                    if (!result.error.isEmpty()) {
                        emit failed(result.error);
                    } else {
                        emit recognized(result.text, result.languageTag, result.elapsedMs);
                    }
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
    return true;
}

} // namespace snipnexs
