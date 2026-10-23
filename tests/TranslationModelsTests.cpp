#include "translate/TranslationModels.h"

#include <QTextStream>

namespace {

bool expectSpec(
    snipnexs::TranslationEngineId engine,
    const QString& sourceTag,
    const QString& targetLanguage,
    const QString& expectedId,
    const char* name)
{
    snipnexs::TranslationModelSpec spec;
    const bool found = snipnexs::findTranslationModelSpec(
        engine, sourceTag, targetLanguage, spec);
    if (found && spec.id == expectedId) {
        return true;
    }
    QTextStream(stderr) << name << " failed: found=" << found
                        << " id=" << (found ? spec.id : QString()) << '\n';
    return false;
}

bool expectMissing(
    snipnexs::TranslationEngineId engine,
    const QString& sourceTag,
    const QString& targetLanguage,
    const char* name)
{
    snipnexs::TranslationModelSpec spec;
    if (!snipnexs::findTranslationModelSpec(
            engine, sourceTag, targetLanguage, spec)) {
        return true;
    }
    QTextStream(stderr) << name << " failed: unexpected id=" << spec.id << '\n';
    return false;
}

} // namespace

int main()
{
    using snipnexs::TranslationEngineId;
    bool ok = true;

    // Default engine: one small package per direction.
    ok &= expectSpec(TranslationEngineId::OpusMt, QString(), "zh-CN",
        "opus-mt-en-zh-int8", "opus to zh via target hint");
    ok &= expectSpec(TranslationEngineId::OpusMt, QString(), "en",
        "opus-mt-zh-en-int8", "opus to en via target hint");
    ok &= expectSpec(TranslationEngineId::OpusMt, "zh", QString(),
        "opus-mt-zh-en-int8", "opus to en via OCR tag");
    ok &= expectSpec(TranslationEngineId::OpusMt, "en-US", QString(),
        "opus-mt-en-zh-int8", "opus to zh via OCR tag");

    // High-quality engine: one GGUF package covers both directions.
    ok &= expectSpec(TranslationEngineId::LlamaHyMT2, QString(), "zh-CN",
        "hy-mt2-1.8b-q4", "llama to zh");
    ok &= expectSpec(TranslationEngineId::LlamaHyMT2, "zh", QString(),
        "hy-mt2-1.8b-q4", "llama to en via OCR tag");
    ok &= expectSpec(TranslationEngineId::LlamaHyMT2, "en-US", QString(),
        "hy-mt2-1.8b-q4", "llama to zh via OCR tag");
    ok &= expectSpec(TranslationEngineId::LlamaHyMT2, "ja", QString(),
        "hy-mt2-1.8b-q4", "llama accepts unknown source");

    // A fixed OPUS-MT pair cannot cover an unknown OCR language, and a
    // same-language request is not a translation.
    ok &= expectMissing(TranslationEngineId::OpusMt, "ja", QString(),
        "opus rejects unknown source");
    ok &= expectMissing(TranslationEngineId::OpusMt, "zh", "zh-CN",
        "opus rejects same language");
    ok &= expectMissing(TranslationEngineId::LlamaHyMT2, "zh", "zh-CN",
        "llama rejects same language");
    ok &= expectMissing(TranslationEngineId::LlamaHyMT2, "en", "ja",
        "llama rejects unsupported target");
    ok &= expectMissing(TranslationEngineId::OpusMt, QString(), QString(),
        "opus needs a direction");

    // Target resolution: an explicit target wins; otherwise the OCR tag
    // decides; only the prompt-driven engine defaults an unknown source.
    ok &= snipnexs::normalizedTranslationLanguage(QStringLiteral("zh-Hans"))
        == QStringLiteral("zh");
    ok &= snipnexs::normalizedTranslationLanguage(QStringLiteral("en-US"))
        == QStringLiteral("en");
    ok &= snipnexs::normalizedTranslationLanguage(QStringLiteral("ja"))
        .isEmpty();
    ok &= snipnexs::translationTargetLanguage(
              TranslationEngineId::OpusMt, QStringLiteral("en"), QString())
        == QStringLiteral("zh");
    ok &= snipnexs::translationTargetLanguage(
              TranslationEngineId::OpusMt, QStringLiteral("ja"), QString())
        .isEmpty();
    ok &= snipnexs::translationTargetLanguage(
              TranslationEngineId::LlamaHyMT2, QStringLiteral("ja"), QString())
        == QStringLiteral("zh");
    ok &= snipnexs::translationTargetLanguage(
              TranslationEngineId::LlamaHyMT2, QStringLiteral("en"),
              QStringLiteral("zh-CN"))
        == QStringLiteral("zh");
    ok &= snipnexs::translationTargetLanguage(
              TranslationEngineId::LlamaHyMT2, QStringLiteral("en"),
              QStringLiteral("ja"))
        .isEmpty();

    QTextStream(stdout) << "translation-models-tests: "
                        << (ok ? "ok" : "failed") << '\n';
    return ok ? 0 : 1;
}
