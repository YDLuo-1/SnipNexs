#include "translate/BrowserTranslation.h"

#include <QTextStream>
#include <QUrlQuery>

int main()
{
    const QUrl url = snipnexs::browserTranslationUrl(
        QStringLiteral("hello 世界"), QStringLiteral("zh-CN"));
    const QUrlQuery query(url);

    bool ok = url.host() == QStringLiteral("translate.google.com");
    ok &= query.queryItemValue(QStringLiteral("sl")) == QStringLiteral("auto");
    ok &= query.queryItemValue(QStringLiteral("tl")) == QStringLiteral("zh-CN");
    ok &= query.queryItemValue(QStringLiteral("text")) == QStringLiteral("hello 世界");
    ok &= snipnexs::browserTranslationUrl({}, QStringLiteral("en")).isEmpty();

    QTextStream(stdout) << (ok ? "browser translation: ok\n" : "browser translation: failed\n");
    return ok ? 0 : 1;
}
