#include "capture/CaptureOverlay.h"

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QPushButton>
#include <QSignalSpy>
#include <QTextStream>
#include <QTest>

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
    bool ok = cropped.size() == QSize(100, 60) && cropped.devicePixelRatio() == 1.0;

    QImage rendered(overlay.size(), QImage::Format_ARGB32_Premultiplied);
    rendered.fill(Qt::transparent);
    overlay.render(&rendered);
    const QColor selectedPixel = rendered.pixelColor(25, 25);
    const QColor dimmedPixel = rendered.pixelColor(5, 5);
    ok &= selectedPixel.lightness() > dimmedPixel.lightness();

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

    QTextStream(stdout) << (ok ? "capture overlay: ok\n" : "capture overlay: failed\n");
    return ok ? 0 : 1;
}
