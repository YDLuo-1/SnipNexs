#pragma once

#include <QStringList>

namespace snipnexs {

// Splits text into segments the local translation model can handle
// before the engine applies its own token budget.
// Segments concatenate back to the original text exactly, so callers
// can pass through segments they choose not to translate.
[[nodiscard]] QStringList splitTranslationSegments(const QString& text, int maxCharacters = 300);

} // namespace snipnexs
