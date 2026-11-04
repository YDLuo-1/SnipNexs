#include "recorder/NativeScreenRecorder.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QTextStream>
#include <QtGui/qscreen_platform.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <thread>

namespace {

struct DecodedTrack {
    int samples = 0;
    qint64 endMs = -1;
};

HRESULT decodeTrack(IMFSourceReader* reader, DWORD stream, bool audio, DecodedTrack& decoded)
{
    HRESULT hr = reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
    if (FAILED(hr)) { return hr; }
    hr = reader->SetStreamSelection(stream, TRUE);
    if (FAILED(hr)) { return hr; }
    Microsoft::WRL::ComPtr<IMFMediaType> type;
    if (FAILED(hr = MFCreateMediaType(&type))) { return hr; }
    type->SetGUID(MF_MT_MAJOR_TYPE, audio ? MFMediaType_Audio : MFMediaType_Video);
    type->SetGUID(MF_MT_SUBTYPE, audio ? MFAudioFormat_PCM : MFVideoFormat_NV12);
    if (FAILED(hr = reader->SetCurrentMediaType(stream, nullptr, type.Get()))) { return hr; }
    PROPVARIANT start;
    PropVariantInit(&start);
    start.vt = VT_I8;
    start.hVal.QuadPart = 0;
    if (FAILED(hr = reader->SetCurrentPosition(GUID_NULL, start))) { return hr; }
    LONGLONG previous = -1;
    for (;;) {
        Microsoft::WRL::ComPtr<IMFSample> sample;
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        hr = reader->ReadSample(stream, 0, nullptr, &flags, &timestamp, &sample);
        if (FAILED(hr)) { return hr; }
        if (sample) {
            DWORD bytes = 0;
            LONGLONG duration = 0;
            sample->GetTotalLength(&bytes);
            sample->GetSampleDuration(&duration);
            if (bytes == 0 || timestamp < previous) { return E_FAIL; }
            previous = timestamp;
            decoded.endMs = (timestamp + duration) / 10'000;
            ++decoded.samples;
        }
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) { break; }
    }
    return decoded.samples > 0 ? S_OK : E_FAIL;
}

qint64 mp4DurationMs(const QString& path, HRESULT& probeError, bool& hasAudio,
    DecodedTrack& video, DecodedTrack& audio)
{
    probeError = MFStartup(MF_VERSION);
    if (FAILED(probeError)) {
        return -1;
    }
    Microsoft::WRL::ComPtr<IMFSourceReader> reader;
    const std::wstring nativePath = QDir::toNativeSeparators(path).toStdWString();
    qint64 durationMs = -1;
    probeError = MFCreateSourceReaderFromURL(nativePath.c_str(), nullptr, &reader);
    if (SUCCEEDED(probeError)) {
        Microsoft::WRL::ComPtr<IMFMediaType> audioType;
        hasAudio = SUCCEEDED(reader->GetNativeMediaType(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, &audioType));
        PROPVARIANT duration;
        PropVariantInit(&duration);
        probeError = reader->GetPresentationAttribute(
            static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE), MF_PD_DURATION, &duration);
        if (SUCCEEDED(probeError)) {
            if (duration.vt == VT_UI8) {
                durationMs = static_cast<qint64>(duration.uhVal.QuadPart / 10'000);
            } else if (duration.vt == VT_I8) {
                durationMs = duration.hVal.QuadPart / 10'000;
            }
        }
        PropVariantClear(&duration);
        if (SUCCEEDED(probeError)) {
            probeError = decodeTrack(reader.Get(), MF_SOURCE_READER_FIRST_VIDEO_STREAM, false, video);
        }
        if (SUCCEEDED(probeError) && hasAudio) {
            probeError = decodeTrack(reader.Get(), MF_SOURCE_READER_FIRST_AUDIO_STREAM, true, audio);
        }
    }
    reader.Reset();
    MFShutdown();
    return durationMs;
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QScreen* screen = QGuiApplication::primaryScreen();
    // Settings parsing runs everywhere, before any capture-permission skip.
    bool settingsOk =
        snipnexs::recordingAudioFromString(QStringLiteral("microphone"))
            == snipnexs::RecordingAudio::Microphone
        && snipnexs::recordingAudioFromString(QStringLiteral("none"))
            == snipnexs::RecordingAudio::None
        && snipnexs::recordingAudioFromString(QString())
            == snipnexs::RecordingAudio::SystemLoopback
        && snipnexs::recordingAudioFromString(QStringLiteral("junk"))
            == snipnexs::RecordingAudio::SystemLoopback;
    if (!settingsOk) {
        QTextStream(stderr) << "recording audio mode parsing failed\n";
        return 1;
    }

    if (screen == nullptr || QGuiApplication::platformName() == QStringLiteral("offscreen")) {
        QTextStream(stdout) << "native recorder: skipped (no interactive display)\n";
        return 77;
    }

    const QString directory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString outputPath = QDir(directory).filePath(
        QStringLiteral("SnipNexs-NativeRecorderTest-%1.mp4").arg(QCoreApplication::applicationPid()));
    QFile::remove(outputPath);
    QFile output(outputPath);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream(stderr) << "native recorder: failed to create temporary output\n";
        return 1;
    }
    output.close();

    const QSize pixelSize = screen->grabWindow(0).size();
    snipnexs::RecordingSettings settings;
    settings.screenName = screen->name();
    if (const auto* nativeScreen = screen->nativeInterface<QNativeInterface::QWindowsScreen>()) {
        settings.monitorHandle = reinterpret_cast<quintptr>(nativeScreen->handle());
    }
    const bool fullScreen = qEnvironmentVariableIsSet("SNIPNEXS_RECORDER_TEST_FULL_SCREEN");
    settings.sourceRect = QRect(
        0,
        0,
        (fullScreen ? pixelSize.width() : std::min(640, pixelSize.width())) & ~1,
        (fullScreen ? pixelSize.height() : std::min(360, pixelSize.height())) & ~1);
    settings.outputPath = outputPath;
    settings.frameRate = 30;
    settings.audio = snipnexs::recordingAudioFromString(
        qEnvironmentVariable("SNIPNEXS_RECORDER_TEST_AUDIO"));
    bool durationValid = false;
    const int requestedDuration = qEnvironmentVariableIntValue(
        "SNIPNEXS_RECORDER_TEST_MS", &durationValid);
    settings.maximumDurationMs = durationValid
        ? std::clamp(requestedDuration, 500, 120'000)
        : 1500;

    const std::atomic_bool stopRequested = false;
    snipnexs::RecordingResult result;
    std::jthread worker([&]() {
        result = snipnexs::recordScreenNative(settings, stopRequested);
    });
    worker.join();
    const bool hasMp4Header = [&outputPath]() {
        QFile file(outputPath);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
        const QByteArray header = file.read(64);
        return header.contains("ftyp") && (file.size() > 4096);
    }();

    HRESULT durationError = S_OK;
    bool hasAudio = false;
    DecodedTrack video, audio;
    const qint64 durationMs = mp4DurationMs(outputPath, durationError, hasAudio, video, audio);
    QFile::remove(outputPath);
    if (!result.succeeded && result.unavailable) {
        QTextStream(stdout) << "native recorder: skipped (" << result.error << ")\n";
        return 77;
    }
    const int toleranceMs = std::max(500, settings.maximumDurationMs / 10);
    const bool durationOk = std::abs(durationMs - settings.maximumDurationMs) <= toleranceMs;
    const bool decodedOk = SUCCEEDED(durationError) && video.samples > 0
        && (!hasAudio || (audio.samples > 0 && std::abs(audio.endMs - video.endMs) < 500));
    const bool audioOk = settings.audio == snipnexs::RecordingAudio::None
        ? !hasAudio : (!qEnvironmentVariableIsSet("SNIPNEXS_RECORDER_REQUIRE_AUDIO")
            || (result.audioCaptured && hasAudio));
    const bool ok = result.succeeded && result.submittedFrames > 0 && hasMp4Header
        && durationOk && audioOk && decodedOk;
    QTextStream(stdout)
        << "native recorder: " << (ok ? "ok" : "failed")
        << " (size=" << settings.sourceRect.width() << 'x' << settings.sourceRect.height()
        << ", captured=" << result.capturedFrames
        << ", submitted=" << result.submittedFrames
        << ", elapsed_ms=" << result.elapsedMs
        << ", duration_ms=" << durationMs
        << ", duration_hr=0x" << QString::number(static_cast<quint32>(durationError), 16)
        << ", bytes=" << result.outputBytes
        << ", audio_captured=" << result.audioCaptured << ", audio_stream=" << hasAudio
        << ", decoded_video=" << video.samples << ", decoded_audio=" << audio.samples
        << ", video_end_ms=" << video.endMs << ", audio_end_ms=" << audio.endMs
        << ", compatibility=" << result.compatibilityCapture
        << ", error=" << result.error << ")\n";
    return ok ? 0 : 1;
}
