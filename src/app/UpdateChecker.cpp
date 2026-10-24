#include "UpdateChecker.h"
#include "UpdateVersion.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace snipnexs {

namespace {
constexpr auto kLatestReleaseUrl =
    "https://api.github.com/repos/YDLuo-1/SnipNexs/releases/latest";
} // namespace

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , network_(new QNetworkAccessManager(this))
{
}

UpdateChecker::~UpdateChecker() = default;

void UpdateChecker::check()
{
    QNetworkRequest request{QUrl(QString::fromLatin1(kLatestReleaseUrl))};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setTransferTimeout(15'000);
    QNetworkReply* reply = network_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit finished(false, QString(), reply->errorString());
            return;
        }
        const QJsonObject release = QJsonDocument::fromJson(reply->readAll()).object();
        const QString tag = release.value(QStringLiteral("tag_name")).toString();
        if (tag.isEmpty()) {
            emit finished(false, QString(), tr("发布信息中没有版本号。"));
            return;
        }
        emit finished(
            isNewerReleaseVersion(QStringLiteral(SNIPNEXS_VERSION), tag), tag, QString());
    });
}

} // namespace snipnexs
