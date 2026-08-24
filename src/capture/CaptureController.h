#pragma once

#include <QObject>
#include <QImage>
#include <QList>
#include <QPointer>

#include "ocr/OcrService.h"

class QImage;
class QString;

namespace snipnexs {

class CaptureOverlay;
class MainWindow;

class CaptureController final : public QObject
{
    Q_OBJECT

public:
    explicit CaptureController(MainWindow& mainWindow, QObject* parent = nullptr);

public slots:
    void startCapture();

signals:
    void recordRegionRequested(
        quintptr monitorHandle, const QString& screenName, const QRect& pixelRect);

private:
    void captureAfterUiSettles();
    void copyImage(const QImage& image);
    void recognizeImage(const QImage& image);
    void recordRegion(const QRect& pixelRect);
    void pinImage(const QImage& image);
    void saveImage(const QImage& image);
    void finishCapture(bool restoreMainWindow);
    void rememberSuccessfulCapture(const QImage& image);
    void reportFailure(const QString& message);
    void setMainWindowCaptureExclusion(bool excluded);

    MainWindow& mainWindow_;
    OcrService ocrService_;
    QPointer<CaptureOverlay> overlay_;
    bool capturePending_ = false;
    bool mainWindowWasVisible_ = false;
    bool mainWindowCaptureExcluded_ = false;
    QList<QImage> captureHistory_;
    qsizetype captureHistoryBytes_ = 0;
    QString activeScreenName_;
    quintptr activeMonitorHandle_ = 0;
};

} // namespace snipnexs
