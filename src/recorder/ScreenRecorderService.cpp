#include "ScreenRecorderService.h"

#include "NativeScreenRecorder.h"

#include <QMetaObject>

namespace snipnexs {

ScreenRecorderService::ScreenRecorderService(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<RecordingResult>();
}

ScreenRecorderService::~ScreenRecorderService()
{
    stop();
    wait();
}

bool ScreenRecorderService::start(const RecordingSettings& settings)
{
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return false;
    }
    if (worker_.joinable()) {
        worker_.join();
    }
    stopRequested_.store(false, std::memory_order_relaxed);
    result_ = {};
    worker_ = std::jthread([this, settings]() {
        result_ = recordScreenNative(settings, stopRequested_, [this]() {
            QMetaObject::invokeMethod(this, &ScreenRecorderService::ready, Qt::QueuedConnection);
        });
        QMetaObject::invokeMethod(this, [this, result = result_]() {
            running_.store(false, std::memory_order_release);
            emit completed(result);
        }, Qt::QueuedConnection);
    });
    return true;
}

void ScreenRecorderService::stop()
{
    stopRequested_.store(true, std::memory_order_relaxed);
}

RecordingResult ScreenRecorderService::wait()
{
    if (worker_.joinable()) {
        worker_.join();
    }
    return result_;
}

bool ScreenRecorderService::isRunning() const
{
    return running_.load(std::memory_order_acquire);
}

} // namespace snipnexs
