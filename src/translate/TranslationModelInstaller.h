#pragma once

#include <QObject>
#include <QPointer>
#include <QCryptographicHash>
#include <memory>

#include "translate/TranslationModels.h"

class QNetworkAccessManager;
class QNetworkReply;
class QSaveFile;
class QLockFile;

namespace snipnexs {

// Downloads a translation model package file by file, verifies each file's
// SHA-256, and finishes by writing the package manifest atomically. Lives on
// the GUI thread; QNetworkAccessManager keeps the actual I/O asynchronous.
// A package lock excludes other installers and deletion; each QSaveFile owns
// its temporary file. modelsRoot allows isolated stores, including tests.
class TranslationModelInstaller final : public QObject
{
    Q_OBJECT

public:
    explicit TranslationModelInstaller(
        TranslationModelSpec spec, QObject* parent = nullptr, QString modelsRoot = {});
    ~TranslationModelInstaller() override;

    void start();
    void cancel();

    [[nodiscard]] QString packageId() const { return spec_.id; }

signals:
    void progress(qint64 bytesReceived, qint64 bytesTotal, const QString& currentFile);
    void finished(bool success, const QString& error);

private:
    void downloadNextFile();
    bool consumeAvailableData();
    void finish(bool success, const QString& error = {});
    void resetDownload();

    TranslationModelSpec spec_;
    QNetworkAccessManager* networkManager_ = nullptr;
    QPointer<QNetworkReply> activeReply_;
    std::unique_ptr<QSaveFile> activeFile_;
    std::unique_ptr<QLockFile> packageLock_;
    QCryptographicHash hash_{QCryptographicHash::Sha256};
    QString modelsRoot_;
    QString activeFileName_;
    int nextFileIndex_ = 0;
    qint64 receivedBytes_ = 0;
    qint64 totalBytes_ = 0;
    qint64 fileBytes_ = 0;
    bool finished_ = false;
};

} // namespace snipnexs
