#pragma once

#include <QRect>
#include <QString>

namespace snipnexs {

enum class RecordingAudio {
    None,
    SystemLoopback,
    Microphone,
};

// Parses the persisted `recording/audio` value; unknown values fall back to
// the default (system loopback).
[[nodiscard]] inline RecordingAudio recordingAudioFromString(const QString& value)
{
    if (value == QStringLiteral("microphone")) {
        return RecordingAudio::Microphone;
    }
    if (value == QStringLiteral("none")) {
        return RecordingAudio::None;
    }
    return RecordingAudio::SystemLoopback;
}

struct RecordingSettings
{
    QString screenName;
    quintptr monitorHandle = 0;
    QRect sourceRect;
    QString outputPath;
    int frameRate = 30;
    int bitrate = 0;
    int maximumDurationMs = 0;
    RecordingAudio audio = RecordingAudio::SystemLoopback;
    int audioBitrate = 128'000;
};

struct RecordingResult
{
    bool succeeded = false;
    bool unavailable = false;
    bool audioCaptured = false;
    QString error;
    qint64 elapsedMs = 0;
    qint64 outputBytes = 0;
    int capturedFrames = 0;
    int submittedFrames = 0;
};

} // namespace snipnexs
