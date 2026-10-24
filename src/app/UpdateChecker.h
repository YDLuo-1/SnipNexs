#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

namespace snipnexs {

// Checks the GitHub "latest" release for a newer tagged version. The request
// is only issued on explicit user action or when the opt-in
// `update/checkAtStartup` setting is enabled; nothing is uploaded.
class UpdateChecker final : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker() override;

    void check();

signals:
    // version carries the tag name ("v0.9.0") when found; error is non-empty
    // when the check itself failed.
    void finished(bool newerFound, const QString& version, const QString& error);

private:
    QNetworkAccessManager* network_ = nullptr;
};

} // namespace snipnexs
