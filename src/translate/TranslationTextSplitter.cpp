#include "TranslationTextSplitter.h"

#include <QChar>
#include <QString>

namespace snipnexs {

namespace {

bool isCjkCharacter(QChar character)
{
    const char16_t code = character.unicode();
    return (code >= 0x3040 && code <= 0x30FF)   // kana
        || (code >= 0xAC00 && code <= 0xD7AF)   // hangul
        || (code >= 0x3400 && code <= 0x4DBF)   // CJK extension A
        || (code >= 0x4E00 && code <= 0x9FFF)   // CJK unified
        || (code >= 0xF900 && code <= 0xFAFF);  // CJK compatibility
}

bool isSentenceTerminator(QChar character)
{
    switch (character.unicode()) {
    case u'.': case u'!': case u'?': case u';':
    case u'。': case u'！': case u'？': case u'…': case u'；':
        return true;
    default:
        return false;
    }
}

} // namespace

QStringList splitTranslationSegments(const QString& text, int maxCharacters)
{
    if (maxCharacters <= 0) {
        maxCharacters = 300;
    }

    QStringList segments;
    QString current;
    const qsizetype total = text.size();
    for (qsizetype index = 0; index < total; ++index) {
        const QChar character = text.at(index);
        current.append(character);

        const bool nextIsCjk = index + 1 < total
            && isCjkCharacter(text.at(index + 1));
        const bool nextIsSpace = index + 1 < total
            && text.at(index + 1).isSpace();
        const bool sentenceEnd = isSentenceTerminator(character)
            && (nextIsCjk || nextIsSpace || index + 1 >= total);
        const bool forcedEnd = character.isSpace()
            || current.size() >= maxCharacters;
        if (sentenceEnd || forcedEnd) {
            segments.append(current);
            current.clear();
        }
    }
    if (!current.isEmpty()) {
        segments.append(current);
    }
    return segments;
}

} // namespace snipnexs
