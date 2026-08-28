#include "translate/LocalTranslation.h"
#include "translate/TranslationTextSplitter.h"

#include <QChar>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QtGlobal>

namespace {

bool modelFilesExist(const QString& directory)
{
    return QDir(directory).exists()
        && QFile::exists(QDir(directory).filePath(QStringLiteral("model.bin")))
        && QFile::exists(QDir(directory).filePath(QStringLiteral("source.spm")))
        && QFile::exists(QDir(directory).filePath(QStringLiteral("target.spm")));
}

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
    const QString modelDirectory = qEnvironmentVariable("SNIPNEXS_TEST_MODEL_DIR");
    if (modelDirectory.isEmpty() || !modelFilesExist(modelDirectory)) {
        QTextStream(stdout) << "local-translation-tests: skipped (no model)\n";
        return 77;
    }

    bool ok = true;
    QString error;
    QElapsedTimer timer;
    timer.start();
    local_translation::Session* session =
        local_translation::openSession(modelDirectory, &error);
    const qint64 loadMs = timer.elapsed();
    if (session == nullptr) {
        QTextStream(stderr) << "session open failed: " << error << '\n';
        QTextStream(stdout) << "local-translation-tests: failed\n";
        return 1;
    }

    const QString source = QStringLiteral(
        "Hello, world. This is a local translation test.");
    const QStringList segments = snipnexs::splitTranslationSegments(source);
    timer.restart();
    const QString translated =
        local_translation::translateSegments(*session, segments, &error);
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
    if (translated.trimmed().isEmpty()) {
        ok = false;
    }

    // Whitespace-only segments must pass through untouched.
    const QString passthrough = local_translation::translateSegments(
        *session,
        { QStringLiteral("  \n  "), QStringLiteral("One more sentence. "),
          QStringLiteral("   ") },
        &error);
    if (!error.isEmpty() || !passthrough.contains(QChar(0x4E00))
        || passthrough.trimmed().size() < 2) {
        QTextStream(stderr) << "passthrough batch failed: " << passthrough
                            << " (" << error << ")\n";
        ok = false;
    }

    local_translation::closeSession(session);

    QTextStream(stdout)
        << "local-translation-tests: " << (ok ? "ok" : "failed") << '\n'
        << "output: " << translated << '\n'
        << "load-ms: " << loadMs << " translate-ms: " << translateMs << '\n';
    return ok ? 0 : 1;
}
