#include "translate/TranslationTextSplitter.h"

#include <QTextStream>

namespace {

bool expectEqual(
    const QStringList& actual,
    const QStringList& expected,
    const char* name)
{
    if (actual == expected) {
        return true;
    }
    QTextStream(stderr) << name << " failed:\n  actual: "
                        << actual.join(QLatin1String(" | "))
                        << "\n  expected: "
                        << expected.join(QLatin1String(" | ")) << '\n';
    return false;
}

bool expectRestoresOriginal(const QString& text, const char* name)
{
    const QStringList segments = snipnexs::splitTranslationSegments(text);
    if (segments.join(QString()) == text) {
        return true;
    }
    QTextStream(stderr) << name << " failed: segments do not restore the text\n";
    return false;
}

} // namespace

int main()
{
    using snipnexs::splitTranslationSegments;
    bool ok = true;

    ok &= expectEqual(
        splitTranslationSegments(QStringLiteral("你好。世界！")),
        { QStringLiteral("你好。"), QStringLiteral("世界！") },
        "cjk terminators split without spaces");

    ok &= expectEqual(
        splitTranslationSegments(QStringLiteral("Hello world. How are you?")),
        { QStringLiteral("Hello world."), QStringLiteral(" How are you?") },
        "ascii terminators split on following space");

    ok &= expectEqual(
        splitTranslationSegments(QStringLiteral("version 3.5 stays")),
        { QStringLiteral("version 3.5 stays") },
        "decimal points do not split");

    ok &= expectEqual(
        splitTranslationSegments(QString(700, u'汉')),
        { QString(300, u'汉'), QString(300, u'汉'), QString(100, u'汉') },
        "long cjk runs hard-split at the character limit");

    ok &= expectEqual(
        splitTranslationSegments(QStringLiteral("第一行\n第二行")),
        { QStringLiteral("第一行\n"), QStringLiteral("第二行") },
        "newlines force a boundary");

    ok &= expectEqual(
        splitTranslationSegments(QString()),
        {},
        "empty text yields no segments");

    ok &= expectRestoresOriginal(
        QStringLiteral("Hello! 世界的 middle...。end\ttab  3.14 e.g. done!"),
        "exact partition property");

    ok &= expectRestoresOriginal(QString(1000, u'a'), "long ascii run restores");

    QTextStream(stdout) << (ok ? "translation-splitter-tests: ok\n"
                               : "translation-splitter-tests: failed\n");
    return ok ? 0 : 1;
}
