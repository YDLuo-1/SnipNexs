#include "CaptureController.h"

#include "CaptureOverlay.h"
#include "app/MainWindow.h"
#include "ocr/OcrResultWindow.h"
#include "pin/PinWindow.h"

#include <QClipboard>
#include <QCursor>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QGuiApplication>
#include <QImage>
#include <QPixmap>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>
#include <utility>

namespace snipnexs {

CaptureController::CaptureController(MainWindow& mainWindow, QObject* parent)
    : QObject(parent)
    , mainWindow_(mainWindow)
{
    connect(&ocrService_, &OcrService::recognized, this,
        [this](const QString& text, const QString& languageTag, qint64 elapsedMs) {
            auto* window = new OcrResultWindow(text, languageTag, elapsedMs);
            window->show();
            window->raise();
            window->activateWindow();
            mainWindow_.setCaptureStatus(
                QStringLiteral("OCR 完成：%1，%2 ms，识别 %3 个字符。")
                    .arg(languageTag)
                    .arg(elapsedMs)
                    .arg(text.size()));
        });
    connect(&ocrService_, &OcrService::failed, this, [this](const QString& message) {
        mainWindow_.setCaptureStatus(message);
        mainWindow_.showNotification(QStringLiteral("OCR 失败"), message);
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
    mainWindow_.hide();
    QTimer::singleShot(120, this, &CaptureController::captureAfterUiSettles);
}

void CaptureController::captureAfterUiSettles()
{
    capturePending_ = false;
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        reportFailure(QStringLiteral("未找到可用显示器。"));
        return;
    }

    QElapsedTimer timer;
    timer.start();
    QPixmap screenshot = screen->grabWindow(0);
    if (screenshot.isNull()) {
        reportFailure(QStringLiteral("屏幕捕获失败。请检查远程桌面或系统权限。"));
        return;
    }

    auto* overlay = new CaptureOverlay(std::move(screenshot));
    overlay_ = overlay;
    overlay->setGeometry(screen->geometry());
    connect(overlay, &CaptureOverlay::copyRequested, this, &CaptureController::copyImage);
    connect(overlay, &CaptureOverlay::ocrRequested, this, &CaptureController::recognizeImage);
    connect(overlay, &CaptureOverlay::pinRequested, this, &CaptureController::pinImage);
    connect(overlay, &CaptureOverlay::saveRequested, this, &CaptureController::saveImage);
    connect(overlay, &CaptureOverlay::canceled, this, [this]() {
        finishCapture(mainWindowWasVisible_);
    });
    connect(overlay, &QObject::destroyed, this, [this]() { overlay_ = nullptr; });
    overlay->show();

    mainWindow_.setCaptureStatus(
        QStringLiteral("已捕获 %1，耗时 %2 ms。拖出选区后复制或保存。")
            .arg(screen->name())
            .arg(timer.elapsed()));
}

void CaptureController::recognizeImage(const QImage& image)
{
    finishCapture(mainWindowWasVisible_);
    if (!ocrService_.recognize(image)) {
        const QString message = QStringLiteral("已有 OCR 任务正在运行，请稍后再试。");
        mainWindow_.setCaptureStatus(message);
        mainWindow_.showNotification(QStringLiteral("OCR 忙碌"), message);
        return;
    }
    mainWindow_.setCaptureStatus(
        QStringLiteral("正在本地识别 %1 × %2 像素图像……")
            .arg(image.width())
            .arg(image.height()));
}

void CaptureController::pinImage(const QImage& image)
{
    auto* pin = new PinWindow(image);
    QPoint position = QCursor::pos() + QPoint(18, 18);
    if (QScreen* screen = QGuiApplication::screenAt(QCursor::pos())) {
        const QRect available = screen->availableGeometry();
        position.setX(std::clamp(
            position.x(),
            available.left(),
            std::max(available.left(), available.right() - pin->width() + 1)));
        position.setY(std::clamp(
            position.y(),
            available.top(),
            std::max(available.top(), available.bottom() - pin->height() + 1)));
    }
    pin->move(position);
    pin->show();

    mainWindow_.setCaptureStatus(
        QStringLiteral("已创建 %1 × %2 像素贴图。滚轮缩放，拖动移动，右键关闭。")
            .arg(image.width())
            .arg(image.height()));
    finishCapture(false);
}

void CaptureController::copyImage(const QImage& image)
{
    QGuiApplication::clipboard()->setImage(image);
    mainWindow_.setCaptureStatus(
        QStringLiteral("已复制 %1 × %2 像素截图到剪贴板。")
            .arg(image.width())
            .arg(image.height()));
    mainWindow_.showNotification(
        QStringLiteral("截图已复制"),
        QStringLiteral("%1 × %2 像素").arg(image.width()).arg(image.height()));
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
        QStringLiteral("保存截图"),
        initialPath,
        QStringLiteral("PNG 图片 (*.png);;JPEG 图片 (*.jpg *.jpeg);;BMP 图片 (*.bmp)"));

    if (fileName.isEmpty()) {
        if (overlay_ != nullptr) {
            overlay_->show();
            overlay_->activateWindow();
        }
        return;
    }

    if (!image.save(fileName)) {
        reportFailure(QStringLiteral("无法保存截图：%1").arg(QDir::toNativeSeparators(fileName)));
        return;
    }

    mainWindow_.setCaptureStatus(
        QStringLiteral("已保存 %1 × %2 像素截图：%3")
            .arg(image.width())
            .arg(image.height())
            .arg(QDir::toNativeSeparators(fileName)));
    mainWindow_.showNotification(QStringLiteral("截图已保存"), QDir::toNativeSeparators(fileName));
    finishCapture(false);
}

void CaptureController::finishCapture(bool restoreMainWindow)
{
    if (overlay_ != nullptr) {
        overlay_->close();
        overlay_ = nullptr;
    }
    if (restoreMainWindow) {
        mainWindow_.showAndActivate();
    }
}

void CaptureController::reportFailure(const QString& message)
{
    mainWindow_.setCaptureStatus(message);
    mainWindow_.showNotification(QStringLiteral("截图失败"), message);
    finishCapture(mainWindowWasVisible_);
}

} // namespace snipnexs
