// Compile the production audio capture implementation into this device probe;
// no monitor capture is requested and no audio samples are stored on disk.
#include "recorder/NativeScreenRecorder.cpp"
#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (!qEnvironmentVariableIsSet("SNIPNEXS_AUDIO_DEVICE_TESTS")) { return 77; }
    const bool microphone = app.arguments().contains(QStringLiteral("--microphone"));
    try {
        snipnexs::ApartmentGuard apartment;
        std::atomic_bool stop = false;
        snipnexs::AudioCaptureSource source(microphone
            ? snipnexs::RecordingAudio::Microphone : snipnexs::RecordingAudio::SystemLoopback,
            stop, [] { return false; });
        std::jthread timer([&stop] {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            stop = true;
        });
        int64_t samples = 0;
        int64_t previous = -1;
        bool monotonic = true;
        for (;;) {
            auto chunk = source.next();
            if (chunk.samples.empty()) { break; }
            monotonic &= chunk.timestampHns > previous;
            previous = chunk.timestampHns;
            samples += chunk.samples.size();
        }
        const bool ok = monotonic && samples >= source.sampleRate() * source.channels();
        QTextStream(stdout) << (microphone ? "microphone" : "loopback")
            << ": " << (ok ? "ok" : "failed") << ", samples=" << samples
            << ", rate=" << source.sampleRate() << ", channels=" << source.channels() << '\n';
        return ok ? 0 : 1;
    } catch (const winrt::hresult_error& error) {
        QTextStream(stderr) << "audio device failure: 0x" << Qt::hex << error.code().value
            << ' ' << QString::fromWCharArray(error.message().c_str()) << '\n';
        return 1;
    }
}
