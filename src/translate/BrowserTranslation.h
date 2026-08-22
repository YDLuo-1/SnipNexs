#pragma once

#include <QUrl>

class QString;

namespace snipnexs {

[[nodiscard]] QUrl browserTranslationUrl(const QString& text, const QString& targetLanguage);

} // namespace snipnexs
