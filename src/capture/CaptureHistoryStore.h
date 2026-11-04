#pragma once

#include <QImage>
#include <QList>
#include <QString>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace snipnexs {

// Owns the small, local-only screenshot history used by the capture overlay.
// The store deliberately exposes images, not file names, so the controller and
// UI never depend on the on-disk representation.
class CaptureHistoryStore final
{
public:
    explicit CaptureHistoryStore(QString directory = {});

    QList<QImage> load();
    bool append(const QImage& image);

    [[nodiscard]] QString directory() const { return directory_; }

private:
    QString directory_;
};

// The pending history follows the same retention limit as the in-memory history.
// Shutdown drains the retained entries before the store is destroyed.
class CaptureHistoryWriter final
{
public:
    explicit CaptureHistoryWriter(QString directory, std::function<void()> onError);
    ~CaptureHistoryWriter();
    void append(QImage image);

private:
    void run();
    CaptureHistoryStore store_;
    std::function<void()> onError_;
    std::mutex mutex_;
    std::condition_variable changed_;
    std::deque<QImage> pending_;
    qsizetype pendingBytes_ = 0;
    bool closing_ = false;
    std::jthread worker_;
};

} // namespace snipnexs
