#pragma once

#include <QObject>
#include <QPointer>

#include "translate/TranslationModels.h"

class QNetworkAccessManager;
class QNetworkReply;
class QFile;

namespace snipnexs {

// Downloads a translation model package file by file, verifies each file's
// SHA-256, and finishes by writing the package manifest atomically. Lives on
// the GUI thread; QNetworkAccessManager keeps the actual I/O asynchronous.
// One installer per package; cancel() aborts pending downloads and removes
// partial files.
class TranslationModelInstaller final : public QObject
{
    Q_OBJECT

public:
    explicit TranslationModelInstaller(
        TranslationModelSpec spec, QObject* parent = nullptr);
    ~TranslationModelInstaller() override;

    void start();
    void cancel();

    [[nodiscard]] QString packageId() const { return spec_.id; }

signals:
    void progress(qint64 bytesReceived, qint64 bytesTotal, const QString& currentFile);
    void finished(bool success, const QString& error);

private:
    void downloadNextFile();
    void cleanupPartialFiles();

    TranslationModelSpec spec_;
    QNetworkAccessManager* networkManager_ = nullptr;
    QPointer<QNetworkReply> activeReply_;
    QFile* activeFile_ = nullptr;
    QString activeFileName_;
    int nextFileIndex_ = 0;
    qint64 receivedBytes_ = 0;
    qint64 totalBytes_ = 0;
    bool canceled_ = false;
    bool finished_ = false;
};

} // namespace snipnexs
