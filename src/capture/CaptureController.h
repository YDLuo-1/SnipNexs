#pragma once

#include <QObject>
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

private:
    void captureAfterUiSettles();
    void copyImage(const QImage& image);
    void recognizeImage(const QImage& image);
    void pinImage(const QImage& image);
    void saveImage(const QImage& image);
    void finishCapture(bool restoreMainWindow);
    void reportFailure(const QString& message);

    MainWindow& mainWindow_;
    OcrService ocrService_;
    QPointer<CaptureOverlay> overlay_;
    bool capturePending_ = false;
    bool mainWindowWasVisible_ = false;
};

} // namespace snipnexs
