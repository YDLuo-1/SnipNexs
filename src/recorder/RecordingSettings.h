#pragma once

#include <QRect>
#include <QString>

namespace snipnexs {

struct RecordingSettings
{
    QString screenName;
    quintptr monitorHandle = 0;
    QRect sourceRect;
    QString outputPath;
    int frameRate = 30;
    int bitrate = 0;
    int maximumDurationMs = 0;
};

struct RecordingResult
{
    bool succeeded = false;
    bool unavailable = false;
    QString error;
    qint64 elapsedMs = 0;
    qint64 outputBytes = 0;
    int capturedFrames = 0;
    int submittedFrames = 0;
};

} // namespace snipnexs
