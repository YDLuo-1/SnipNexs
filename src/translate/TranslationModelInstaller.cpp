#include "TranslationModelInstaller.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <utility>

namespace snipnexs {

namespace {

QString partSuffix(const QString& fileName)
{
    return fileName + QStringLiteral(".part");
}

} // namespace

TranslationModelInstaller::TranslationModelInstaller(
    TranslationModelSpec spec, QObject* parent)
    : QObject(parent)
    , spec_(std::move(spec))
{
    networkManager_ = new QNetworkAccessManager(this);
    networkManager_->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
}

TranslationModelInstaller::~TranslationModelInstaller()
{
    canceled_ = true;
    if (activeReply_) {
        activeReply_->abort();
    }
    delete activeFile_;
}

void TranslationModelInstaller::start()
{
    const QDir packageDir = translationModelDirectory(spec_.id);
    if (!QDir().mkpath(packageDir.absolutePath())) {
        emit finished(false, tr("无法创建模型目录:%1").arg(packageDir.absolutePath()));
        return;
    }

    totalBytes_ = 0;
    for (const TranslationModelFile& file : spec_.files) {
        totalBytes_ += file.sizeBytes > 0 ? file.sizeBytes : 0;
    }

    downloadNextFile();
}

void TranslationModelInstaller::cancel()
{
    if (finished_) {
        return;
    }
    canceled_ = true;
    if (activeReply_) {
        activeReply_->abort();
    }
    cleanupPartialFiles();
    finished_ = true;
    emit finished(false, QString());
}

void TranslationModelInstaller::downloadNextFile()
{
    if (canceled_) {
        return;
    }
    if (nextFileIndex_ >= spec_.files.size()) {
        if (writeTranslationModelManifest(spec_)) {
            finished_ = true;
            emit finished(true, QString());
        } else {
            emit finished(false, tr("无法写入模型清单文件。"));
        }
        return;
    }

    const TranslationModelFile& file = spec_.files.at(nextFileIndex_);
    activeFileName_ = file.fileName;
    const QUrl url(QDir(translationModelsBaseUrl()).filePath(file.remotePath));
    if (!url.isValid()) {
        emit finished(false, tr("模型下载地址无效:%1").arg(file.remotePath));
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    activeReply_ = networkManager_->get(request);

    const QDir packageDir = translationModelDirectory(spec_.id);
    const QString partPath = packageDir.filePath(partSuffix(activeFileName_));
    activeFile_ = new QFile(partPath);
    if (!activeFile_->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        delete activeFile_;
        activeFile_ = nullptr;
        activeReply_->deleteLater();
        activeReply_ = nullptr;
        emit finished(false, tr("无法写入文件:%1").arg(partPath));
        return;
    }

    connect(activeReply_, &QNetworkReply::readyRead, this, [this]() {
        if (!activeReply_ || !activeFile_) {
            return;
        }
        const QByteArray chunk = activeReply_->readAll();
        if (activeFile_->write(chunk) != chunk.size()) {
            activeReply_->abort();
            return;
        }
        receivedBytes_ += chunk.size();
        emit progress(receivedBytes_, totalBytes_, activeFileName_);
    });

    connect(activeReply_, &QNetworkReply::finished, this, [this, partPath]() {
        QFile* file = activeFile_;
        activeFile_ = nullptr;
        QNetworkReply* reply = activeReply_;
        activeReply_ = nullptr;
        const TranslationModelFile& spec = spec_.files.at(nextFileIndex_);
        const QString finalPath = QFileInfo(partPath).dir().filePath(spec.fileName);

        const auto fail = [this, &partPath, &file](const QString& message) {
            delete file;
            file = nullptr;
            QFile::remove(partPath);
            emit finished(false, message);
        };

        if (canceled_) {
            delete file;
            QFile::remove(partPath);
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            fail(tr("下载失败(%1):%2").arg(spec.fileName, reply->errorString()));
            return;
        }

        const QByteArray remaining = reply->readAll();
        reply->deleteLater();
        if (!remaining.isEmpty()) {
            if (file->write(remaining) != remaining.size()) {
                fail(tr("写入文件失败:%1").arg(partPath));
                return;
            }
            receivedBytes_ += remaining.size();
            emit progress(receivedBytes_, totalBytes_, activeFileName_);
        }
        file->flush();
        delete file;
        file = nullptr;

        QCryptographicHash hash(QCryptographicHash::Sha256);
        QFile verifyFile(partPath);
        if (!verifyFile.open(QIODevice::ReadOnly) || !hash.addData(&verifyFile)) {
            fail(tr("无法校验下载文件:%1").arg(spec.fileName));
            return;
        }
        verifyFile.close();

        const bool sizeOk = spec.sizeBytes <= 0
            || QFileInfo(partPath).size() == spec.sizeBytes;
        const bool hashOk = QString::fromLatin1(hash.result().toHex())
            .compare(QString::fromLatin1(spec.sha256), Qt::CaseInsensitive) == 0;
        if (!sizeOk || !hashOk) {
            fail(tr("模型文件校验失败:%1").arg(spec.fileName));
            return;
        }

        QFile::remove(finalPath);
        if (!QFile::rename(partPath, finalPath)) {
            fail(tr("无法保存模型文件:%1").arg(spec.fileName));
            return;
        }

        ++nextFileIndex_;
        downloadNextFile();
    });
}

void TranslationModelInstaller::cleanupPartialFiles()
{
    const QDir packageDir = translationModelDirectory(spec_.id);
    const QStringList entries = packageDir.entryList(
        QStringList() << QStringLiteral("*.part"), QDir::Files);
    for (const QString& entry : entries) {
        QFile::remove(packageDir.filePath(entry));
    }
}

} // namespace snipnexs
