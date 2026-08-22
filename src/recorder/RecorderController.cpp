#include "RecorderController.h"

#include "RecordingIndicator.h"
#include "app/MainWindow.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QCursor>
#include <QStandardPaths>

#include <Windows.h>

namespace snipnexs {

RecorderController::RecorderController(MainWindow& mainWindow, QObject* parent)
    : QObject(parent)
    , mainWindow_(mainWindow)
{
    connect(&service_, &ScreenRecorderService::ready, this, [this]() {
        if (indicator_ != nullptr) {
            indicator_->setRecordingReady();
        }
        mainWindow_.setCaptureStatus(tr("正在录制区域视频。点击悬浮条停止并完成 MP4。"));
    });
    connect(&service_, &ScreenRecorderService::completed,
        this, &RecorderController::handleCompleted);
}

RecorderController::~RecorderController()
{
    service_.stop();
    service_.wait();
    if (!partialPath_.isEmpty()) {
        QFile::remove(partialPath_);
    }
}

void RecorderController::startRegion(
    quintptr monitorHandle, const QString& screenName, const QRect& pixelRect)
{
    if (service_.isRunning()) {
        mainWindow_.showNotification(
            tr("正在录屏"), tr("请先停止当前录制。"));
        return;
    }

    const QString videos = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    const QString suggestedName = QStringLiteral("SnipNexs-%1.mp4")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    finalPath_ = QFileDialog::getSaveFileName(
        nullptr,
        tr("保存区域录屏"),
        QDir(videos).filePath(suggestedName),
        tr("MP4 视频 (*.mp4)"));
    if (finalPath_.isEmpty()) {
        mainWindow_.showAndActivate();
        return;
    }
    if (!finalPath_.endsWith(QStringLiteral(".mp4"), Qt::CaseInsensitive)) {
        finalPath_ += QStringLiteral(".mp4");
    }

    partialPath_ = QStringLiteral("%1.snipnexs-%2.partial.mp4")
        .arg(finalPath_)
        .arg(QDateTime::currentMSecsSinceEpoch());
    QFile partial(partialPath_);
    if (!partial.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        mainWindow_.setCaptureStatus(
            tr("无法创建录屏文件：%1").arg(QDir::toNativeSeparators(partialPath_)));
        mainWindow_.showAndActivate();
        return;
    }
    partial.close();

    RecordingSettings settings;
    settings.screenName = screenName;
    settings.monitorHandle = monitorHandle;
    settings.sourceRect = pixelRect;
    settings.outputPath = partialPath_;
    settings.frameRate = 30;

    if (!service_.start(settings)) {
        QFile::remove(partialPath_);
        mainWindow_.showNotification(
            tr("正在录屏"), tr("请先停止当前录制。"));
        return;
    }

    auto* indicator = new RecordingIndicator;
    indicator_ = indicator;
    connect(indicator, &RecordingIndicator::stopRequested, this, &RecorderController::stop);
    connect(indicator, &QObject::destroyed, this, [this]() { indicator_ = nullptr; });
    indicator->show();
    indicator->move(QCursor::pos() + QPoint(18, 18));
    mainWindow_.hide();
    mainWindow_.setCaptureStatus(tr("正在启动 Windows GPU 录屏与 H.264 编码器…"));
}

void RecorderController::stop()
{
    if (indicator_ != nullptr) {
        indicator_->setStopping();
    }
    service_.stop();
}

void RecorderController::handleCompleted(const RecordingResult& result)
{
    if (indicator_ != nullptr) {
        indicator_->finish();
        indicator_ = nullptr;
    }

    if (!result.succeeded) {
        QFile::remove(partialPath_);
        mainWindow_.setCaptureStatus(result.error);
        mainWindow_.showNotification(tr("录屏失败"), result.error);
        mainWindow_.showAndActivate();
        return;
    }

    const std::wstring partialNative = QDir::toNativeSeparators(partialPath_).toStdWString();
    const std::wstring finalNative = QDir::toNativeSeparators(finalPath_).toStdWString();
    if (!MoveFileExW(
            partialNative.c_str(),
            finalNative.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const QString message = tr(
            "MP4 已生成，但无法原子移动到目标路径（错误 %1）。临时文件保留在：%2")
            .arg(GetLastError())
            .arg(QDir::toNativeSeparators(partialPath_));
        mainWindow_.setCaptureStatus(message);
        mainWindow_.showNotification(tr("录屏文件未移动"), message);
        mainWindow_.showAndActivate();
        return;
    }

    const QString nativePath = QDir::toNativeSeparators(finalPath_);
    mainWindow_.setCaptureStatus(
        tr("录屏完成：处理 %1 帧，%2 秒，%3 MiB。%4")
            .arg(result.submittedFrames)
            .arg(result.elapsedMs / 1000.0, 0, 'f', 1)
            .arg(result.outputBytes / 1024.0 / 1024.0, 0, 'f', 1)
            .arg(nativePath));
    mainWindow_.showNotification(tr("录屏已保存"), nativePath);
    mainWindow_.showAndActivate();
}

} // namespace snipnexs
