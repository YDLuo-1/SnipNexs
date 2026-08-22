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

qint64 mp4DurationMs(const QString& path, HRESULT& probeError)
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
    }
    MFShutdown();
    return durationMs;
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen == nullptr) {
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
    bool durationValid = false;
    const int requestedDuration = qEnvironmentVariableIntValue(
        "SNIPNEXS_RECORDER_TEST_MS", &durationValid);
    settings.maximumDurationMs = durationValid
        ? std::clamp(requestedDuration, 500, 15'000)
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
    const qint64 durationMs = mp4DurationMs(outputPath, durationError);
    QFile::remove(outputPath);
    if (!result.succeeded && result.unavailable) {
        QTextStream(stdout) << "native recorder: skipped (" << result.error << ")\n";
        return 77;
    }
    const bool durationOk = durationMs >= settings.maximumDurationMs / 2
        && durationMs <= settings.maximumDurationMs * 3 / 2;
    const bool ok = result.succeeded && result.submittedFrames > 0 && hasMp4Header && durationOk;
    QTextStream(stdout)
        << "native recorder: " << (ok ? "ok" : "failed")
        << " (size=" << settings.sourceRect.width() << 'x' << settings.sourceRect.height()
        << ", captured=" << result.capturedFrames
        << ", submitted=" << result.submittedFrames
        << ", elapsed_ms=" << result.elapsedMs
        << ", duration_ms=" << durationMs
        << ", duration_hr=0x" << QString::number(static_cast<quint32>(durationError), 16)
        << ", bytes=" << result.outputBytes
        << ", error=" << result.error << ")\n";
    return ok ? 0 : 1;
}
