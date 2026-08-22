#include "BrowserTranslation.h"

#include <QString>
#include <QUrlQuery>

namespace snipnexs {

QUrl browserTranslationUrl(const QString& text, const QString& targetLanguage)
{
    if (text.trimmed().isEmpty() || targetLanguage.isEmpty()) {
        return {};
    }

    QUrl url(QStringLiteral("https://translate.google.com/"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("sl"), QStringLiteral("auto"));
    query.addQueryItem(QStringLiteral("tl"), targetLanguage);
    query.addQueryItem(QStringLiteral("text"), text.left(4000));
    query.addQueryItem(QStringLiteral("op"), QStringLiteral("translate"));
    url.setQuery(query);
    return url;
}

} // namespace snipnexs
