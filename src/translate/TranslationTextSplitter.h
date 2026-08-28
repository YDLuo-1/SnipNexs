#pragma once

#include <QStringList>

namespace snipnexs {

// Splits text into segments the local translation model can handle
// (OPUS-MT models degrade beyond a few hundred tokens per segment).
// Segments concatenate back to the original text exactly, so callers
// can pass through segments they choose not to translate.
[[nodiscard]] QStringList splitTranslationSegments(const QString& text, int maxCharacters = 300);

} // namespace snipnexs
