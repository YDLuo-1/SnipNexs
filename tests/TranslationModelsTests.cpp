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

    QTextStream(stdout) << "translation-models-tests: "
                        << (ok ? "ok" : "failed") << '\n';
    return ok ? 0 : 1;
}
