#include "translate/TranslationModels.h"

#include <QDir>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QLockFile>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace {

bool expectSpec(
    const QString& sourceTag,
    const QString& targetLanguage,
    const QString& expectedId,
    const char* name)
{
    snipnexs::TranslationModelSpec spec;
    const bool found = snipnexs::findTranslationModelSpec(
        sourceTag, targetLanguage, spec);
    if (found && spec.id == expectedId) {
        return true;
    }
    QTextStream(stderr) << name << " failed: found=" << found
                        << " id=" << (found ? spec.id : QString()) << '\n';
    return false;
}

bool expectMissing(
    const QString& sourceTag,
    const QString& targetLanguage,
    const char* name)
{
    snipnexs::TranslationModelSpec spec;
    if (!snipnexs::findTranslationModelSpec(
            sourceTag, targetLanguage, spec)) {
        return true;
    }
    QTextStream(stderr) << name << " failed: unexpected id=" << spec.id << '\n';
    return false;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QDir().mkpath(QStringLiteral("test-temp"));
    QTemporaryDir temporary(QDir::current().filePath(QStringLiteral("test-temp/models-XXXXXX")));
    if (!temporary.isValid()) {
        QTextStream(stderr) << "Cannot create isolated model directory\n";
        return 1;
    }
    const QString root = temporary.path();
    bool ok = true;
    ok &= snipnexs::translationSourceLanguage("Hello world.") == "en";
    ok &= snipnexs::translationSourceLanguage(QStringLiteral("打开 OCR 文件")) == "zh";
    ok &= snipnexs::translationSourceLanguage("12345").isEmpty();
    ok &= snipnexs::translationSourceLanguage(QStringLiteral("こんにちは")).isEmpty();

    // High-quality engine: one GGUF package covers both directions.
    ok &= expectSpec(QString(), "zh-CN",
        "hy-mt2-1.8b-q4", "llama to zh");
    ok &= expectSpec("zh", QString(),
        "hy-mt2-1.8b-q4", "llama to en via OCR tag");
    ok &= expectSpec("en-US", QString(),
        "hy-mt2-1.8b-q4", "llama to zh via OCR tag");
    ok &= expectSpec("ja", QString(),
        "hy-mt2-1.8b-q4", "llama accepts unknown source");

    ok &= expectSpec(QString(), QString(), "hy-mt2-1.8b-q4", "default model");
    ok &= snipnexs::knownTranslationModels().size() == 1;
    ok &= expectMissing("zh", "zh-CN", "reject same language");
    ok &= expectMissing("en", "ja", "reject unsupported target");

    // Target resolution: an explicit target wins; otherwise the OCR tag
    // decides; an unknown source defaults to Chinese.
    ok &= snipnexs::normalizedTranslationLanguage(QStringLiteral("zh-Hans"))
        == QStringLiteral("zh");
    ok &= snipnexs::normalizedTranslationLanguage(QStringLiteral("en-US"))
        == QStringLiteral("en");
    ok &= snipnexs::normalizedTranslationLanguage(QStringLiteral("ja"))
        .isEmpty();
    ok &= snipnexs::translationTargetLanguage(
              QStringLiteral("ja"), QString())
        == QStringLiteral("zh");
    ok &= snipnexs::translationTargetLanguage(
              QStringLiteral("en"),
              QStringLiteral("zh-CN"))
        == QStringLiteral("zh");
    ok &= snipnexs::translationTargetLanguage(
              QStringLiteral("en"),
              QStringLiteral("ja"))
        .isEmpty();

    // Every write is inside this test's temporary directory.
    auto spec = snipnexs::knownTranslationModels().first();
    const QString packageDirectory = snipnexs::translationModelDirectory(spec.id, root);
    ok &= QDir().mkpath(packageDirectory);
    for (auto& file : spec.files) {
        file.sizeBytes = 1;
        QFile placeholder(QDir(packageDirectory).filePath(file.fileName));
        ok &= placeholder.open(QIODevice::WriteOnly);
        ok &= placeholder.write("x") == 1;
    }
    ok &= snipnexs::writeTranslationModelManifest(spec, root);
    ok &= snipnexs::isTranslationModelInstalled(spec, root);
    // A manifest alone cannot bless truncated files as a real catalog model.
    ok &= !snipnexs::isTranslationModelInstalled(snipnexs::knownTranslationModels().first(), root);
    ok &= snipnexs::translationModelStatuses(root).size()
        == snipnexs::knownTranslationModels().size();

    QString error;
    QLockFile installLock(snipnexs::translationModelLockPath(spec.id, root));
    ok &= installLock.tryLock(0);
    ok &= !snipnexs::removeTranslationModel(spec.id, &error, root);
    ok &= QFileInfo::exists(packageDirectory);
    installLock.unlock();
    const QString keeper = QDir(root).filePath(QStringLiteral("keep-me"));
    ok &= QDir().mkpath(keeper);
    ok &= snipnexs::removeTranslationModel(spec.id, &error, root);
    ok &= !QFileInfo::exists(packageDirectory);
    ok &= QFileInfo::exists(keeper);
    error.clear();
    ok &= !snipnexs::removeTranslationModel(QStringLiteral("../keep-me"), &error, root);
    ok &= !error.isEmpty();

    QTextStream(stdout) << "translation-models-tests: "
                        << (ok ? "ok" : "failed") << '\n';
    return ok ? 0 : 1;
}
