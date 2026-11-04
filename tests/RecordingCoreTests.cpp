#include "recorder/AudioBuffer.h"
#include "recorder/NativeScreenRecorder.h"
#include "recorder/RecordingOutput.h"
#include "recorder/ScreenRecorderService.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <chrono>
#include <limits>

namespace snipnexs {
// Deterministic encoder boundary: no screen capture or audio device is opened.
RecordingResult recordScreenNative(const RecordingSettings& settings,
    const std::atomic_bool& stop, const std::function<void()>& ready)
{
    if (ready) { ready(); }
    while (!stop.load()) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    QFile output(settings.outputPath);
    RecordingResult result;
    result.succeeded = output.open(QIODevice::WriteOnly) && output.write("completed-video") == 15;
    return result;
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    bool ok = true;
    try {
        const std::vector<int16_t> samples{0, 123, -456, 32767, -32768};
        const auto buffer = snipnexs::pcmBuffer(samples);
        ok &= buffer.Length() == samples.size() * sizeof(int16_t);
        auto reader = winrt::Windows::Storage::Streams::DataReader::FromBuffer(buffer);
        std::vector<uint8_t> bytes(buffer.Length());
        reader.ReadBytes(bytes);
        ok &= std::memcmp(bytes.data(), samples.data(), bytes.size()) == 0;
        snipnexs::AudioBuffer queue;
        queue.setCapacity(6);
        ok &= queue.append(samples);
        ok &= !queue.appendSilence(2);
        ok &= queue.take(2) == std::vector<int16_t>({0, 123});
        ok &= queue.appendSilence(3);
        ok &= !queue.append(samples);
        ok &= queue.take(10) == std::vector<int16_t>({-456, 32767, -32768, 0, 0, 0});
        ok &= queue.empty();
        ok &= snipnexs::audioClockHns(48'000, 48'000) == 10'000'000;
        ok &= snipnexs::audioClockHns(2'000'000'000'000, 10'000'000) == 2'000'000'000'000;
    } catch (const winrt::hresult_error& error) {
        QTextStream(stderr) << "PCM buffer HRESULT=" << Qt::hex << error.code().value << '\n';
        ok = false;
    }

    QDir().mkpath(QStringLiteral("test-temp"));
    QTemporaryDir temporary(QDir::current().filePath(QStringLiteral("test-temp/recording-XXXXXX")));
    if (!temporary.isValid()) { return 1; }
    QString pending = QDir(temporary.path()).filePath(QStringLiteral("capture.partial.mp4"));
    const QString final = QDir(temporary.path()).filePath(QStringLiteral("capture.mp4"));
    snipnexs::RecordingSettings settings;
    settings.outputPath = pending;
    snipnexs::ScreenRecorderService service;
    int delivered = 0;
    QObject::connect(&service, &snipnexs::ScreenRecorderService::completed,
        &app, [&](const auto&) { ++delivered; });
    ok &= service.start(settings);
    service.stop();
    const auto result = service.wait();
    ok &= result.succeeded && delivered == 0;
    ok &= !service.start(settings); // A queued completion still owns the output path.
    ok &= snipnexs::finishRecordingOutput(result, pending, final).isEmpty();
    ok &= pending.isEmpty() && QFile::exists(final);
    app.processEvents();
    ok &= delivered == 1 && !service.isRunning();

    pending = QDir(temporary.path()).filePath(QStringLiteral("recover.partial.mp4"));
    QFile partial(pending);
    ok &= partial.open(QIODevice::WriteOnly);
    ok &= partial.write("recoverable") == 11;
    partial.close();
    const QString recoverable = pending;
    ok &= !snipnexs::finishRecordingOutput(result, pending,
        QDir(temporary.path()).filePath(QStringLiteral("missing/output.mp4"))).isEmpty();
    ok &= pending.isEmpty() && QFile::exists(recoverable);
    pending = recoverable;
    snipnexs::RecordingResult failed;
    failed.error = QStringLiteral("encoder failed");
    ok &= snipnexs::finishRecordingOutput(failed, pending, final).contains(QDir::toNativeSeparators(recoverable));
    ok &= QFile::exists(recoverable) && QFile::exists(final);
    QTextStream(stdout) << "recording core: " << (ok ? "ok" : "failed") << '\n';
    return ok ? 0 : 1;
}
