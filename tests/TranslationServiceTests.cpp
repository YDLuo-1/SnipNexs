#include "translate/TranslationService.h"
#include "translate/LlamaTranslation.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTextStream>
#include <QTest>
#include <atomic>
#include <chrono>
#include <thread>
#include <memory>

namespace {
std::atomic_bool running{false};
QString run(const QStringList& input, QString* error, const std::atomic_bool* canceled)
{
    running.store(true);
    if (input.join(QString()).contains("slow")) {
        while (!canceled->load()) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    }
    running.store(false);
    if (canceled->load()) { *error = "canceled"; return {}; }
    return input.join(" ");
}
bool waitRunning()
{
    QElapsedTimer timer; timer.start();
    while (!running.load() && timer.elapsed() < 2000) { QTest::qWait(1); }
    return running.load();
}
}

// Substitute inference and model lookup; the worker, queued completion and
// destructor under test are the production TranslationService implementation.
namespace snipnexs {
QString translationTargetLanguage(const QString&, const QString&) { return "zh"; }
bool findTranslationModelSpec(const QString&, const QString&, TranslationModelSpec& spec)
{
    spec.id = "hy-mt2-1.8b-q4";
    spec.files.append({"fake", {}, {}, 0});
    return true;
}
bool isTranslationModelInstalled(const TranslationModelSpec&, const QString&) { return true; }
QString translationModelDirectory(const QString&, const QString&) { return "."; }
}
namespace local_llama {
struct Session {};
Session* openSession(const QString&, QString*, const std::atomic_bool*) { return new Session; }
void closeSession(Session* session) { delete session; }
QString translateSegments(Session&, const QStringList& input, const QString&, QString* error, const std::atomic_bool* flag) { return run(input, error, flag); }
QStringList translateLines(Session&, const QStringList& input, const QString&, QString* error, const std::atomic_bool* flag) { return {run(input, error, flag)}; }
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    bool ok = true;
    {
        auto service = std::make_unique<snipnexs::TranslationService>();
        QSignalSpy canceled(service.get(), &snipnexs::TranslationService::canceled);
        QSignalSpy translated(service.get(), &snipnexs::TranslationService::translated);
        QSignalSpy failed(service.get(), &snipnexs::TranslationService::failed);
        ok &= service->translate("slow", "en", "zh") && waitRunning();
        ok &= !service->translate("second", "en", "zh");
        service->cancel();
        ok &= canceled.wait(2000) && canceled.count() == 1;
        ok &= translated.isEmpty() && failed.isEmpty() && !service->isBusy();
        ok &= service->translate("next", "en", "zh");
        ok &= translated.wait(2000) && translated.count() == 1;
        ok &= service->translateLines({"slow"}, "en", "zh") && waitRunning();
        QElapsedTimer timer; timer.start();
        service.reset();
        ok &= timer.elapsed() < 1000 && !running.load();
        QCoreApplication::processEvents(); // A queued completion must not outlive its service.
    }
    QTextStream(stdout) << "Translation service cancellation/shutdown: " << (ok ? "ok" : "failed") << '\n';
    return ok ? 0 : 1;
}
