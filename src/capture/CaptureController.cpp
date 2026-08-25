#include "CaptureController.h"

#include "CaptureOverlay.h"
#include "app/MainWindow.h"
#include "ocr/OcrResultWindow.h"
#include "pin/PinWindow.h"
#include "platform/windows/CaptureExclusion.h"
#include "platform/windows/WindowTargeting.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QCursor>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileDialog>
#include <QGuiApplication>
#include <QImage>
#include <QPixmap>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>
#include <QtGui/qscreen_platform.h>

#include <dwmapi.h>

#include <algorithm>
#include <utility>

namespace snipnexs {

namespace {

constexpr qsizetype kMaximumCaptureHistoryBytes = 64 * 1024 * 1024;
constexpr qsizetype kMaximumCaptureHistoryEntries = 20;

}

CaptureController::CaptureController(MainWindow& mainWindow, QObject* parent)
    : QObject(parent)
    , mainWindow_(mainWindow)
{
    captureHistory_ = historyStore_.load();
    for (const QImage& image : captureHistory_) {
        captureHistoryBytes_ += image.sizeInBytes();
    }

    connect(&ocrService_, &OcrService::recognized, this,
        [this](const QString& text, const QString& languageTag, qint64 elapsedMs) {
            auto* window = new OcrResultWindow(text, languageTag, elapsedMs);
            window->show();
            window->raise();
            window->activateWindow();
            mainWindow_.setCaptureStatus(
                tr("OCR 完成：%1，%2 ms，识别 %3 个字符。")
                    .arg(languageTag)
                    .arg(elapsedMs)
                    .arg(text.size()));
        });
    connect(&ocrService_, &OcrService::failed, this, [this](const QString& message) {
        mainWindow_.setCaptureStatus(message);
        mainWindow_.showNotification(tr("OCR 失败"), message);
    });
}

void CaptureController::startCapture()
{
    if (overlay_ != nullptr) {
        overlay_->raise();
        overlay_->activateWindow();
        return;
    }
    if (capturePending_) {
        return;
    }

    capturePending_ = true;
    mainWindowWasVisible_ = mainWindow_.isVisible();
    setMainWindowCaptureExclusion(mainWindowWasVisible_);
    mainWindow_.setCaptureActive(true);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    QGuiApplication::sync();
    QTimer::singleShot(120, this, &CaptureController::captureAfterUiSettles);
}

void CaptureController::captureAfterUiSettles()
{
    capturePending_ = false;
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    QGuiApplication::sync();
    DwmFlush();

    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        reportFailure(tr("未找到可用显示器。"));
        return;
    }

    QElapsedTimer timer;
    timer.start();
    QPixmap screenshot = screen->grabWindow(0);
    if (screenshot.isNull()) {
        reportFailure(tr("屏幕捕获失败。请检查远程桌面或系统权限。"));
        return;
    }

    setMainWindowCaptureExclusion(false);
    activeScreenName_ = screen->name();
    if (const auto* nativeScreen = screen->nativeInterface<QNativeInterface::QWindowsScreen>()) {
        activeMonitorHandle_ = reinterpret_cast<quintptr>(nativeScreen->handle());
    } else {
        activeMonitorHandle_ = 0;
    }
    auto* overlay = new CaptureOverlay(std::move(screenshot), captureHistory_);
    overlay_ = overlay;
    overlay->setGeometry(screen->geometry());
    overlay->setWindowTargets(visibleWindowTargets(
        activeMonitorHandle_, screen->geometry().size()));
    connect(overlay, &CaptureOverlay::copyRequested, this, &CaptureController::copyImage);
    connect(overlay, &CaptureOverlay::ocrRequested, this, &CaptureController::recognizeImage);
    connect(overlay, &CaptureOverlay::pinRequested, this, &CaptureController::pinImage);
    connect(overlay, &CaptureOverlay::recordRequested, this, &CaptureController::recordRegion);
    connect(overlay, &CaptureOverlay::saveRequested, this, &CaptureController::saveImage);
    connect(overlay, &CaptureOverlay::canceled, this, [this]() {
        finishCapture(mainWindowWasVisible_);
    });
    connect(overlay, &QObject::destroyed, this, [this]() {
        overlay_ = nullptr;
        setMainWindowCaptureExclusion(false);
        mainWindow_.setCaptureActive(false);
    });
    overlay->show();

    mainWindow_.setCaptureStatus(
        tr("已捕获 %1，耗时 %2 ms。选区可移动并通过八个控制点缩放。")
            .arg(screen->name())
            .arg(timer.elapsed()));
}

void CaptureController::recordRegion(const QRect& pixelRect)
{
    const QString screenName = activeScreenName_;
    const quintptr monitorHandle = activeMonitorHandle_;
    finishCapture(false);
    emit recordRegionRequested(monitorHandle, screenName, pixelRect);
}

void CaptureController::recognizeImage(const QImage& image)
{
    finishCapture(mainWindowWasVisible_);
    if (!ocrService_.recognize(image)) {
        const QString message = tr("已有 OCR 任务正在运行，请稍后再试。");
        mainWindow_.setCaptureStatus(message);
        mainWindow_.showNotification(tr("OCR 忙碌"), message);
        return;
    }
    mainWindow_.setCaptureStatus(
        tr("正在本地识别 %1 × %2 像素图像……")
            .arg(image.width())
            .arg(image.height()));
}

void CaptureController::pinImage(const QImage& image, const QPoint& imageTopLeft)
{
    auto* pin = new PinWindow(image);
    connect(pin, &PinWindow::copyRequested,
        this, &CaptureController::copyPinnedImage);
    connect(pin, &PinWindow::saveRequested,
        this, &CaptureController::savePinnedImage);
    pin->moveImageTopLeft(imageTopLeft);
    pin->show();
    rememberSuccessfulCapture(image);

    mainWindow_.setCaptureStatus(
        tr("已创建 %1 × %2 像素贴图。滚轮缩放，拖动移动，双击左键关闭，右键打开菜单。")
            .arg(image.width())
            .arg(image.height()));
    finishCapture(false);
}

void CaptureController::copyPinnedImage(const QImage& image)
{
    QGuiApplication::clipboard()->setImage(image);
    mainWindow_.setCaptureStatus(
        tr("已复制贴图 %1 × %2 像素到剪贴板。")
            .arg(image.width())
            .arg(image.height()));
    mainWindow_.showNotification(
        tr("贴图已复制"),
        tr("%1 × %2 像素").arg(image.width()).arg(image.height()));
}

void CaptureController::savePinnedImage(const QImage& image)
{
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString suggestedName = QStringLiteral("SnipNexs-%1.png")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    const QString initialPath = QDir(pictures).filePath(suggestedName);
    const QString fileName = QFileDialog::getSaveFileName(
        nullptr,
        tr("保存贴图"),
        initialPath,
        tr("PNG 图片 (*.png);;JPEG 图片 (*.jpg *.jpeg);;BMP 图片 (*.bmp)"));
    if (fileName.isEmpty()) {
        return;
    }

    if (!image.save(fileName)) {
        const QString message = tr("无法保存贴图：%1").arg(QDir::toNativeSeparators(fileName));
        mainWindow_.setCaptureStatus(message);
        mainWindow_.showNotification(tr("贴图保存失败"), message);
        return;
    }

    mainWindow_.setCaptureStatus(
        tr("已保存贴图 %1 × %2 像素：%3")
            .arg(image.width())
            .arg(image.height())
            .arg(QDir::toNativeSeparators(fileName)));
    mainWindow_.showNotification(
        tr("贴图已保存"), QDir::toNativeSeparators(fileName));
}

void CaptureController::copyImage(const QImage& image)
{
    QGuiApplication::clipboard()->setImage(image);
    rememberSuccessfulCapture(image);
    mainWindow_.setCaptureStatus(
        tr("已复制 %1 × %2 像素截图到剪贴板。")
            .arg(image.width())
            .arg(image.height()));
    mainWindow_.showNotification(
        tr("截图已复制"),
        tr("%1 × %2 像素").arg(image.width()).arg(image.height()));
    finishCapture(false);
}

void CaptureController::saveImage(const QImage& image)
{
    if (overlay_ != nullptr) {
        overlay_->hide();
    }

    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString suggestedName = QStringLiteral("SnipNexs-%1.png")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    const QString initialPath = QDir(pictures).filePath(suggestedName);
    const QString fileName = QFileDialog::getSaveFileName(
        nullptr,
        tr("保存截图"),
        initialPath,
        tr("PNG 图片 (*.png);;JPEG 图片 (*.jpg *.jpeg);;BMP 图片 (*.bmp)"));

    if (fileName.isEmpty()) {
        if (overlay_ != nullptr) {
            overlay_->show();
            overlay_->activateWindow();
        }
        return;
    }

    if (!image.save(fileName)) {
        reportFailure(tr("无法保存截图：%1").arg(QDir::toNativeSeparators(fileName)));
        return;
    }

    rememberSuccessfulCapture(image);

    mainWindow_.setCaptureStatus(
        tr("已保存 %1 × %2 像素截图：%3")
            .arg(image.width())
            .arg(image.height())
            .arg(QDir::toNativeSeparators(fileName)));
    mainWindow_.showNotification(tr("截图已保存"), QDir::toNativeSeparators(fileName));
    finishCapture(false);
}

void CaptureController::finishCapture(bool restoreMainWindow)
{
    setMainWindowCaptureExclusion(false);
    if (overlay_ != nullptr) {
        overlay_->close();
        overlay_ = nullptr;
    }
    mainWindow_.setCaptureActive(false);
    if (restoreMainWindow) {
        mainWindow_.showAndActivate();
    }
}

void CaptureController::rememberSuccessfulCapture(const QImage& image)
{
    if (image.isNull()) {
        return;
    }

    captureHistory_.append(image);
    captureHistoryBytes_ += image.sizeInBytes();
    while (captureHistory_.size() > 1
        && (captureHistory_.size() > kMaximumCaptureHistoryEntries
            || captureHistoryBytes_ > kMaximumCaptureHistoryBytes)) {
        captureHistoryBytes_ -= captureHistory_.first().sizeInBytes();
        captureHistory_.removeFirst();
    }
    historyStore_.append(image);
}

void CaptureController::setMainWindowCaptureExclusion(bool excluded)
{
    if (excluded) {
        mainWindowCaptureExcluded_ = setWindowExcludedFromCapture(mainWindow_, true);
        return;
    }
    if (!mainWindowCaptureExcluded_) {
        return;
    }

    const bool reset = setWindowExcludedFromCapture(mainWindow_, false);
    Q_UNUSED(reset);
    mainWindowCaptureExcluded_ = false;
}

void CaptureController::reportFailure(const QString& message)
{
    mainWindow_.setCaptureStatus(message);
    mainWindow_.showNotification(tr("截图失败"), message);
    finishCapture(mainWindowWasVisible_);
}

} // namespace snipnexs
