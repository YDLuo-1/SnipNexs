#include "translate/LlamaTranslation.h"
#include "translate/TranslationTextSplitter.h"

#include <QChar>
#include <QElapsedTimer>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QtGlobal>

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

int main()
{
    const QString modelPath = qEnvironmentVariable("SNIPNEXS_TEST_LLAMA_MODEL");
    if (modelPath.isEmpty() || !QFile::exists(modelPath)) {
        QTextStream(stdout) << "local-llama-tests: skipped (no model)\n";
        return 77;
    }

    bool ok = true;
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

    local_llama::closeSession(session);

    QTextStream(stdout)
        << "local-llama-tests: " << (ok ? "ok" : "failed") << '\n'
        << "output: " << translated << '\n'
        << "load-ms: " << loadMs << " translate-ms: " << translateMs << '\n';
    return ok ? 0 : 1;
}
