#include "capture/CaptureOverlay.h"

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QPushButton>
#include <QSignalSpy>
#include <QTextStream>
#include <QTest>

namespace {

bool containsAnnotationColor(const QImage& image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel.red() > 220 && pixel.green() < 110 && pixel.blue() < 110) {
                return true;
            }
        }
    }
    return false;
}

}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QImage source(400, 300, QImage::Format_ARGB32_Premultiplied);
    source.fill(QColor(160, 120, 80));
    QPixmap pixmap = QPixmap::fromImage(source);
    pixmap.setDevicePixelRatio(2.0);

    snipnexs::CaptureOverlay overlay(pixmap);
    overlay.resize(200, 150);
    overlay.setSelection(QRect(20, 20, 50, 30));
    overlay.show();
    QApplication::processEvents();

    const QImage cropped = overlay.selectedImage();
    bool ok = cropped.size() == QSize(100, 60) && cropped.devicePixelRatio() == 2.0;

    QImage rendered(overlay.size(), QImage::Format_ARGB32_Premultiplied);
    rendered.fill(Qt::transparent);
    overlay.render(&rendered);
    const QColor selectedPixel = rendered.pixelColor(25, 25);
    const QColor dimmedPixel = rendered.pixelColor(5, 5);
    const QColor handlePixel = rendered.pixelColor(20, 20);
    ok &= selectedPixel.lightness() > dimmedPixel.lightness();
    ok &= handlePixel.lightness() > 220;

    QTest::mousePress(&overlay, Qt::LeftButton, Qt::NoModifier, QPoint(40, 30));
    QTest::mouseMove(&overlay, QPoint(60, 50));
    QTest::mouseRelease(&overlay, Qt::LeftButton, Qt::NoModifier, QPoint(60, 50));
    ok &= overlay.selectedPixelRect() == QRect(80, 80, 100, 60);

    QTest::mousePress(&overlay, Qt::LeftButton, Qt::NoModifier, QPoint(89, 69));
    QTest::mouseMove(&overlay, QPoint(109, 79));
    QTest::mouseRelease(&overlay, Qt::LeftButton, Qt::NoModifier, QPoint(109, 79));
    ok &= overlay.selectedPixelRect() == QRect(80, 80, 140, 80);

    snipnexs::CaptureOverlay interactionOverlay(pixmap);
    interactionOverlay.resize(200, 150);
    interactionOverlay.show();
    QApplication::processEvents();

    QSignalSpy copySpy(&interactionOverlay, &snipnexs::CaptureOverlay::copyRequested);
    QTest::mousePress(&interactionOverlay, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
    QTest::mouseMove(&interactionOverlay, QPoint(69, 49));
    QTest::mouseRelease(&interactionOverlay, Qt::LeftButton, Qt::NoModifier, QPoint(69, 49));
    QTest::keyClick(&interactionOverlay, Qt::Key_Return);

    ok &= copySpy.count() == 1;
    if (copySpy.count() == 1) {
        const QImage copied = qvariant_cast<QImage>(copySpy.takeFirst().at(0));
        ok &= copied.size() == QSize(100, 60);
    }

    snipnexs::CaptureOverlay annotationOverlay(pixmap);
    annotationOverlay.resize(200, 150);
    annotationOverlay.setSelection(QRect(20, 20, 80, 60));
    annotationOverlay.setAnnotationTool(snipnexs::AnnotationTool::Rectangle);
    annotationOverlay.show();
    annotationOverlay.activateWindow();
    QApplication::processEvents();

    QTest::mousePress(&annotationOverlay, Qt::LeftButton, Qt::NoModifier, QPoint(25, 25));
    QTest::mouseMove(&annotationOverlay, QPoint(70, 50));
    QTest::mouseRelease(&annotationOverlay, Qt::LeftButton, Qt::NoModifier, QPoint(70, 50));
    const QImage annotated = annotationOverlay.selectedImage();
    ok &= annotated.size() == QSize(160, 120);
    ok &= annotated.pixelColor(10, 10).red() > 200;

    QTest::keyClick(&annotationOverlay, Qt::Key_Z, Qt::ControlModifier);
    const QImage undone = annotationOverlay.selectedImage();
    ok &= undone.pixelColor(10, 10) == QColor(160, 120, 80);

    QSignalSpy ocrSpy(&annotationOverlay, &snipnexs::CaptureOverlay::ocrRequested);
    auto* ocrButton = annotationOverlay.findChild<QPushButton*>(QStringLiteral("ocrButton"));
    ok &= ocrButton != nullptr;
    if (ocrButton != nullptr) {
        QTest::mouseClick(ocrButton, Qt::LeftButton);
        ok &= ocrSpy.count() == 1;
        if (ocrSpy.count() == 1) {
            const QImage ocrImage = qvariant_cast<QImage>(ocrSpy.takeFirst().at(0));
            ok &= ocrImage.size() == QSize(160, 120);
        }
    }

    QSignalSpy recordSpy(&annotationOverlay, &snipnexs::CaptureOverlay::recordRequested);
    auto* recordButton = annotationOverlay.findChild<QPushButton*>(QStringLiteral("recordButton"));
    ok &= recordButton != nullptr;
    if (recordButton != nullptr) {
        QTest::mouseClick(recordButton, Qt::LeftButton);
        ok &= recordSpy.count() == 1;
        if (recordSpy.count() == 1) {
            ok &= qvariant_cast<QRect>(recordSpy.takeFirst().at(0)) == QRect(40, 40, 160, 120);
        }
    }

    QImage firstHistoryImage(60, 40, QImage::Format_ARGB32_Premultiplied);
    firstHistoryImage.fill(QColor(220, 40, 50));
    QImage secondHistoryImage(800, 500, QImage::Format_ARGB32_Premultiplied);
    secondHistoryImage.fill(QColor(40, 180, 70));
    secondHistoryImage.setDevicePixelRatio(2.0);
    snipnexs::CaptureOverlay historyOverlay(
        pixmap, {firstHistoryImage, secondHistoryImage});
    historyOverlay.resize(200, 150);
    historyOverlay.show();
    historyOverlay.activateWindow();
    QApplication::processEvents();

    QTest::keyClick(&historyOverlay, Qt::Key_Comma);
    const QImage latestHistory = historyOverlay.selectedImage();
    ok &= latestHistory.size() == secondHistoryImage.size();
    ok &= latestHistory.devicePixelRatio() == 2.0;
    ok &= latestHistory.pixelColor(latestHistory.rect().center()) == QColor(40, 180, 70);
    auto* historyRecordButton = historyOverlay.findChild<QPushButton*>(
        QStringLiteral("recordButton"));
    ok &= historyRecordButton != nullptr && !historyRecordButton->isEnabled();

    historyOverlay.setAnnotationTool(snipnexs::AnnotationTool::Rectangle);
    QTest::mousePress(&historyOverlay, Qt::LeftButton, Qt::NoModifier, QPoint(35, 40));
    QTest::mouseMove(&historyOverlay, QPoint(95, 80));
    QTest::mouseRelease(&historyOverlay, Qt::LeftButton, Qt::NoModifier, QPoint(95, 80));
    const QImage annotatedHistory = historyOverlay.selectedImage();
    ok &= annotatedHistory.size() == secondHistoryImage.size();
    ok &= annotatedHistory.devicePixelRatio() == 2.0;
    ok &= containsAnnotationColor(annotatedHistory);

    QTest::keyClick(&historyOverlay, Qt::Key_Comma);
    const QImage olderHistory = historyOverlay.selectedImage();
    ok &= olderHistory.size() == firstHistoryImage.size();
    ok &= olderHistory.devicePixelRatio() == 1.0;
    ok &= olderHistory.pixelColor(olderHistory.rect().center()) == QColor(220, 40, 50);

    QTest::keyClick(&historyOverlay, Qt::Key_Period);
    const QImage forwardHistory = historyOverlay.selectedImage();
    ok &= forwardHistory.size() == secondHistoryImage.size();
    QTest::keyClick(&historyOverlay, Qt::Key_Period);
    ok &= historyOverlay.selectedImage().isNull();
    ok &= historyRecordButton != nullptr && historyRecordButton->isEnabled();

    QTextStream(stdout) << (ok ? "capture overlay: ok\n" : "capture overlay: failed\n");
    return ok ? 0 : 1;
}
