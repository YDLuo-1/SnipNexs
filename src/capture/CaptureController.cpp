#include "CaptureController.h"

#include "CaptureOverlay.h"
#include "app/MainWindow.h"

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

#include <utility>

namespace snipnexs {

CaptureController::CaptureController(MainWindow& mainWindow, QObject* parent)
    : QObject(parent)
    , mainWindow_(mainWindow)
{
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
