#include "TranslationModelInstaller.h"

#include <QDir>
#include <QFileInfo>
#include <QLockFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QStorageInfo>
#include <QUrl>
#include <utility>

namespace snipnexs {

TranslationModelInstaller::TranslationModelInstaller(
    TranslationModelSpec spec, QObject* parent, QString modelsRoot)
    : QObject(parent), spec_(std::move(spec)), modelsRoot_(std::move(modelsRoot))
{
    networkManager_ = new QNetworkAccessManager(this);
    networkManager_->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
}

TranslationModelInstaller::~TranslationModelInstaller()
{
    resetDownload();
}

void TranslationModelInstaller::resetDownload()
{
    if (activeReply_) {
        // abort() can emit finished synchronously, including during destruction.
        disconnect(activeReply_, nullptr, this, nullptr);
        activeReply_->abort();
        activeReply_->deleteLater();
        activeReply_ = nullptr;
    }
    activeFile_.reset(); // QSaveFile removes only its own uncommitted temporary file.
}

void TranslationModelInstaller::finish(bool success, const QString& error)
{
    if (finished_) {
        return;
    }
    finished_ = true;
    resetDownload();
    packageLock_.reset();
    emit finished(success, error);
}

void TranslationModelInstaller::start()
{
    if (packageLock_ || activeReply_) {
        return;
    }
    finished_ = false;
    nextFileIndex_ = 0;
    receivedBytes_ = 0;
    totalBytes_ = 0;
    if (spec_.id.isEmpty() || spec_.id.contains(QLatin1Char('/'))
        || spec_.id.contains(QLatin1Char('\\')) || spec_.id.contains(QLatin1Char(':'))
        || spec_.id.contains(QStringLiteral("..")) || spec_.files.isEmpty()) {
        finish(false, tr("模型清单无效。"));
        return;
    }
    for (const auto& file : spec_.files) {
        if (file.sizeBytes <= 0 || file.sha256.size() != 64 || file.fileName.isEmpty()
            || QFileInfo(file.fileName).fileName() != file.fileName
            || file.fileName.contains(QLatin1Char(':')) || file.fileName.contains(QStringLiteral(".."))) {
            finish(false, tr("模型清单无效。"));
            return;
        }
        totalBytes_ += file.sizeBytes;
    }
    const QDir packageDir(translationModelDirectory(spec_.id, modelsRoot_));
    if (!QDir().mkpath(packageDir.absolutePath())) {
        finish(false, tr("无法创建模型目录:%1").arg(packageDir.absolutePath()));
        return;
    }
    packageLock_ = std::make_unique<QLockFile>(translationModelLockPath(spec_.id, modelsRoot_));
    packageLock_->setStaleLockTime(0); // Downloads can outlive the default 30-second lock age.
    if (!packageLock_->tryLock(0)) {
        finish(false, tr("该语言包正在另一个窗口中安装，请等待完成。"));
        return;
    }
    const QStorageInfo storage(packageDir.absolutePath());
    if (storage.isValid() && storage.bytesAvailable() >= 0
        && storage.bytesAvailable() < totalBytes_ + 64 * 1024 * 1024) {
        finish(false, tr("模型目录所在磁盘空间不足。"));
        return;
    }
    downloadNextFile();
}

void TranslationModelInstaller::cancel()
{
    finish(false);
}

bool TranslationModelInstaller::consumeAvailableData()
{
    if (!activeReply_ || !activeFile_) {
        return false;
    }
    const auto& spec = spec_.files.at(nextFileIndex_);
    while (activeReply_->bytesAvailable() > 0) {
        const QByteArray chunk = activeReply_->read(256 * 1024);
        if (chunk.isEmpty()) {
            break;
        }
        if (chunk.size() > spec.sizeBytes - fileBytes_) {
            finish(false, tr("模型文件超过清单大小:%1").arg(spec.fileName));
            return false;
        }
        if (activeFile_->write(chunk) != chunk.size()) {
            finish(false, tr("写入文件失败:%1").arg(spec.fileName));
            return false;
        }
        hash_.addData(chunk);
        fileBytes_ += chunk.size();
        receivedBytes_ += chunk.size();
    }
    emit progress(receivedBytes_, totalBytes_, activeFileName_);
    return !finished_;
}

void TranslationModelInstaller::downloadNextFile()
{
    if (finished_) {
        return;
    }
    if (nextFileIndex_ >= spec_.files.size()) {
        const bool written = writeTranslationModelManifest(spec_, modelsRoot_);
        finish(written, written ? QString() : tr("无法写入模型清单文件。"));
        return;
    }
    const auto& file = spec_.files.at(nextFileIndex_);
    activeFileName_ = file.fileName;
    QUrl base(effectiveModelBaseUrl(spec_));
    if (!base.path().endsWith(QLatin1Char('/'))) {
        base.setPath(base.path() + QLatin1Char('/'));
    }
    const QUrl url = base.resolved(QUrl(file.remotePath));
    if (!url.isValid() || (url.scheme() != QStringLiteral("https")
            && url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("file"))) {
        finish(false, tr("模型下载地址无效:%1").arg(file.remotePath));
        return;
    }
    activeFile_ = std::make_unique<QSaveFile>(
        QDir(translationModelDirectory(spec_.id, modelsRoot_)).filePath(file.fileName));
    if (!activeFile_->open(QIODevice::WriteOnly)) {
        finish(false, tr("无法写入文件:%1").arg(file.fileName));
        return;
    }
    fileBytes_ = 0;
    hash_.reset();
    QNetworkRequest request(url);
    request.setTransferTimeout(30'000);
    activeReply_ = networkManager_->get(request);
    activeReply_->setReadBufferSize(256 * 1024);
    connect(activeReply_, &QNetworkReply::readyRead, this, [this] { consumeAvailableData(); });
    connect(activeReply_, &QNetworkReply::finished, this, [this] {
        if (activeReply_->error() != QNetworkReply::NoError) {
            finish(false, tr("下载失败(%1):%2").arg(activeFileName_, activeReply_->errorString()));
            return;
        }
        if (!consumeAvailableData()) {
            return;
        }
        const auto& file = spec_.files.at(nextFileIndex_);
        if (fileBytes_ != file.sizeBytes || hash_.result().toHex().toLower() != file.sha256.toLower()) {
            finish(false, tr("模型文件校验失败:%1").arg(file.fileName));
            return;
        }
        if (!activeFile_->commit()) {
            finish(false, tr("无法保存模型文件:%1").arg(file.fileName));
            return;
        }
        resetDownload();
        ++nextFileIndex_;
        downloadNextFile();
    });
}

} // namespace snipnexs
