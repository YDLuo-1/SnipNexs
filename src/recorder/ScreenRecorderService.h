#pragma once

#include "RecordingSettings.h"

#include <QObject>

#include <atomic>
#include <thread>

namespace snipnexs {

class ScreenRecorderService final : public QObject
{
    Q_OBJECT

public:
    explicit ScreenRecorderService(QObject* parent = nullptr);
    ~ScreenRecorderService() override;

    [[nodiscard]] bool start(const RecordingSettings& settings);
    void stop();
    void wait();
    [[nodiscard]] bool isRunning() const;

signals:
    void ready();
    void completed(const RecordingResult& result);

private:
    std::atomic_bool running_ = false;
    std::atomic_bool stopRequested_ = false;
    std::jthread worker_;
};

} // namespace snipnexs

Q_DECLARE_METATYPE(snipnexs::RecordingResult)
