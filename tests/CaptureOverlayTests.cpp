#include "capture/CaptureOverlay.h"

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QPixmap>
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

    QTextStream(stdout) << (ok ? "capture overlay: ok\n" : "capture overlay: failed\n");
    return ok ? 0 : 1;
}
