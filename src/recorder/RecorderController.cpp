#include "RecorderController.h"
#include "app/AppSettings.h"

#include "RecordingIndicator.h"
#include "RecordingOutput.h"
#include "app/MainWindow.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
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
    const RecordingResult result = service_.wait();
    const QString error = finishRecordingOutput(result, partialPath_, finalPath_);
    if (!error.isEmpty()) {
        qWarning().noquote() << error;
    }
}

void RecorderController::startRegion(
    quintptr monitorHandle, const QString& screenName, const QRect& pixelRect)
{
    if (service_.isRunning() || !partialPath_.isEmpty()) {
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
        partialPath_.clear();
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
    settings.audio = recordingAudioFromString(
        AppSettings().value(QStringLiteral("recording/audio")).toString());

    if (!service_.start(settings)) {
        QFile::remove(partialPath_);
        partialPath_.clear();
        mainWindow_.showNotification(
            tr("正在录屏"), tr("请先停止当前录制。"));
        mainWindow_.showAndActivate();
        return;
    }

    auto* indicator = new RecordingIndicator;
    indicator_ = indicator;
    connect(indicator, &RecordingIndicator::stopRequested, this, &RecorderController::stop);
    connect(indicator, &QObject::destroyed, this, [this]() { indicator_ = nullptr; });
    indicator->moveToBottomRight(screenName);
    indicator->show();
    mainWindow_.hide();
    mainWindow_.setCaptureStatus(tr("正在启动 Windows 录屏与 H.264 编码器…"));
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

    const QString message = finishRecordingOutput(result, partialPath_, finalPath_);
    if (!result.succeeded || !message.isEmpty()) {
        mainWindow_.setCaptureStatus(message);
        mainWindow_.showNotification(tr("录屏保存异常"), message);
        mainWindow_.showAndActivate();
        return;
    }

    const QString nativePath = QDir::toNativeSeparators(finalPath_);
    mainWindow_.setCaptureStatus(
        tr("录屏完成：处理 %1 帧，%2 秒，%3 MiB。%4")
            .arg(result.submittedFrames)
            .arg(result.elapsedMs / 1000.0, 0, 'f', 1)
            .arg(result.outputBytes / 1024.0 / 1024.0, 0, 'f', 1)
            .arg(nativePath) + (result.warning.isEmpty() ? QString() : QLatin1Char('\n') + result.warning));
    mainWindow_.showNotification(tr("录屏已保存"),
        result.warning.isEmpty() ? nativePath : nativePath + QLatin1Char('\n') + result.warning);
    mainWindow_.showAndActivate();
}

} // namespace snipnexs
