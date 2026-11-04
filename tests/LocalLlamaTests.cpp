#include "translate/LlamaTranslation.h"
#include "translate/TranslationTextSplitter.h"

#include <QChar>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QtGlobal>
#include <ggml-backend.h>
#include <atomic>
#include <chrono>
#include <thread>

namespace {

bool containsCjk(const QString& text)
{
    for (const QChar& character : text) {
        const char16_t code = character.unicode();
        if (code >= 0x4E00 && code <= 0x9FFF) {
            return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    const QString modelPath = qEnvironmentVariable("SNIPNEXS_TEST_LLAMA_MODEL");
    if (modelPath.isEmpty() || !QFile::exists(modelPath)) {
        QTextStream(stdout) << "local-llama-tests: skipped (no model)\n";
        return 77;
    }

    bool ok = true;
    const bool baseline = app.arguments().contains(QStringLiteral("--baseline"));
    if (baseline) {
        const QByteArray backend = QDir(app.applicationDirPath())
            .filePath(QStringLiteral("ggml-cpu-x64.dll")).toUtf8();
        if (!ggml_backend_load(backend.constData())) {
            QTextStream(stderr) << "baseline CPU backend failed to load\n";
            return 1;
        }
    }
    QString error;
    QElapsedTimer timer;
    timer.start();
    local_llama::Session* session =
        local_llama::openSession(modelPath, &error);
    const qint64 loadMs = timer.elapsed();
    if (session == nullptr) {
        QTextStream(stderr) << "session open failed: " << error << '\n';
        QTextStream(stdout) << "local-llama-tests: failed\n";
        return 1;
    }

    const QString source = QStringLiteral(
        "Hello, world. This is a local translation test.");
    timer.restart();
    const QString translated = local_llama::translateSegments(
        *session,
        snipnexs::splitTranslationSegments(source),
        QStringLiteral("zh"),
        &error);
    const qint64 translateMs = timer.elapsed();

    if (!error.isEmpty()) {
        QTextStream(stderr) << "translate failed: " << error << '\n';
        ok = false;
    }
    if (!containsCjk(translated)) {
        QTextStream(stderr) << "expected CJK output, got: "
                            << translated << '\n';
        ok = false;
    }

    if (baseline) {
        local_llama::closeSession(session);
        QTextStream(stdout) << "baseline translation: " << translated
            << " (load_ms=" << loadMs << ", translate_ms=" << translateMs << ")\n";
        return ok ? 0 : 1;
    }

    // Repeated batches must not accumulate tokens in the shared context.
    const QStringList repeated = snipnexs::splitTranslationSegments(
        QStringLiteral("One. Two. Three. Four. Five. Six. Seven. Eight."));
    for (int round = 0; round < 2 && ok; ++round) {
        QString repeatedError;
        const QString repeatedOutput = local_llama::translateSegments(
            *session, repeated, QStringLiteral("zh"), &repeatedError);
        if (!repeatedError.isEmpty() || !containsCjk(repeatedOutput)) {
            QTextStream(stderr) << "repeated batch failed (round " << round
                                << "): " << repeatedError << '\n';
            ok = false;
        }
    }

    // A 600-character segment matches TranslationService's Llama split size
    // and exercises the larger context window without crossing n_ctx.
    const QString longSource = QStringLiteral(
        "这是一个用于验证长文本分段和上下文窗口的句子。").repeated(25);
    QString longError;
    const QString longOutput = local_llama::translateSegments(
        *session,
        snipnexs::splitTranslationSegments(longSource, 600),
        QStringLiteral("en"),
        &longError);
    if (!longError.isEmpty() || longOutput.trimmed().isEmpty()) {
        QTextStream(stderr) << "long segment failed: " << longError << '\n';
        ok = false;
    }

    // Line-aligned translation: one entry per input line with blank lines
    // passed through, for the bilingual OCR view.
    const QStringList lineResults = local_llama::translateLines(
        *session,
        { QStringLiteral("Hello again."), QString(),
          QStringLiteral("Second line here.") },
        QStringLiteral("zh"),
        &error);
    if (!error.isEmpty() || lineResults.size() != 3
        || lineResults.at(1) != QString()
        || !containsCjk(lineResults.at(0))
        || !containsCjk(lineResults.at(2))) {
        QTextStream(stderr) << "line-aligned batch failed: "
                            << lineResults.join(QStringLiteral(" | "))
                            << " (" << error << ")\n";
        ok = false;
    }

    // Previously 563 prompt tokens were sent to n_batch=512 and aborted the process.
    const QString overflowSource = QStringLiteral("甲乙丙丁戊己庚辛壬癸").repeated(60);
    const auto overflowResult = local_llama::translateSegments(*session,
        {overflowSource}, QStringLiteral("en"), &error);
    ok &= !overflowResult.isEmpty() || error.contains(QStringLiteral("输出长度限制"));
    QTextStream(stdout) << "batch-overflow regression: " << (error.isEmpty() ? "completed" : error) << '\n';
    // An input larger than the full context must be split before prefill and cancel promptly.
    QTextStream(stdout) << "starting cancellation probe\n" << Qt::flush;
    std::atomic_bool canceled{false};
    std::jthread cancelThread([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        canceled.store(true);
    });
    timer.restart();
    const auto canceledResult = local_llama::translateSegments(*session,
        {overflowSource.repeated(8)}, QStringLiteral("en"), &error, &canceled);
    const auto cancelMs = timer.elapsed();
    ok &= canceledResult.isEmpty() && !error.isEmpty() && cancelMs < 5000;
    QTextStream(stdout) << "canceled call returned in " << cancelMs << " ms\n" << Qt::flush;
    cancelThread.join();
    const auto afterCancel = local_llama::translateSegments(*session,
        {QStringLiteral("Order number 12345.")}, QStringLiteral("zh"), &error);
    ok &= error.isEmpty() && afterCancel.contains(QStringLiteral("12345"));
    QTextStream(stdout) << "cancel-ms: " << cancelMs << " after-cancel: " << afterCancel << '\n';

    local_llama::closeSession(session);

    QTextStream(stdout)
        << "local-llama-tests: " << (ok ? "ok" : "failed") << '\n'
        << "output: " << translated << '\n'
        << "load-ms: " << loadMs << " translate-ms: " << translateMs << '\n';
    return ok ? 0 : 1;
}
