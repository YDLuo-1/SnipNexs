#include "pin/PinWindow.h"

#include <QApplication>
#include <QImage>
#include <QPointingDevice>
#include <QTextStream>
#include <QTest>
#include <QWheelEvent>

namespace {

constexpr int kMarginTotal = 32;

void sendWheel(QWidget& widget, const QPoint& globalPosition, int delta)
{
    const QPoint localPosition = widget.mapFromGlobal(globalPosition);
    QWheelEvent event(
        localPosition,
        globalPosition,
        QPoint(),
        QPoint(0, delta),
        Qt::NoButton,
        Qt::NoModifier,
        Qt::NoScrollPhase,
        false,
        Qt::MouseEventNotSynthesized,
        QPointingDevice::primaryPointingDevice());
    QApplication::sendEvent(&widget, &event);
    QApplication::processEvents();
}
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QImage image(320, 200, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(40, 160, 200));

    snipnexs::PinWindow pin(image);
    pin.move(100, 100);
    pin.show();
    QApplication::processEvents();

    const QSize expectedSize(320 + kMarginTotal, 200 + kMarginTotal);
    const bool initialSizeOk = pin.size() == expectedSize;
    bool ok = initialSizeOk;
    const QRect available = QGuiApplication::primaryScreen()->availableGeometry();
    const QPoint imageTopLeft = available.topLeft() + QPoint(40, 40);
    pin.moveImageTopLeft(imageTopLeft);
    const bool imagePositionOk = pin.pos()
        == imageTopLeft - QPoint(kMarginTotal / 2, kMarginTotal / 2);
    ok &= imagePositionOk;
    const QPoint originalPosition = pin.pos();
    QTest::mousePress(&pin, Qt::LeftButton, Qt::NoModifier, QPoint(40, 30));
    QTest::mouseMove(&pin, QPoint(90, 70));
    QTest::mouseRelease(&pin, Qt::LeftButton, Qt::NoModifier, QPoint(90, 70));
    const bool dragOk = pin.pos() == originalPosition + QPoint(50, 40);
    ok &= dragOk;

    const QSize originalSize = pin.size();
    const QPoint zoomAnchorLocal(96, 72);
    const QPoint zoomAnchorGlobal = pin.mapToGlobal(zoomAnchorLocal);
    sendWheel(pin, zoomAnchorGlobal, 120);
    const QSize firstZoomSize = pin.size();
    const bool firstZoomOk = firstZoomSize.width() > originalSize.width()
        && firstZoomSize.height() > originalSize.height();
    sendWheel(pin, zoomAnchorGlobal, 120);
    const QSize secondZoomSize = pin.size();
    const bool cumulativeZoomOk = secondZoomSize.width() > firstZoomSize.width()
        && secondZoomSize.height() > firstZoomSize.height();
    const QPoint anchoredLocal = pin.mapFromGlobal(zoomAnchorGlobal);
    const qreal sourceXBefore = (zoomAnchorLocal.x() - kMarginTotal / 2.0)
        / (originalSize.width() - kMarginTotal);
    const qreal sourceYBefore = (zoomAnchorLocal.y() - kMarginTotal / 2.0)
        / (originalSize.height() - kMarginTotal);
    const qreal sourceXAfter = (anchoredLocal.x() - kMarginTotal / 2.0)
        / (secondZoomSize.width() - kMarginTotal);
    const qreal sourceYAfter = (anchoredLocal.y() - kMarginTotal / 2.0)
        / (secondZoomSize.height() - kMarginTotal);
    const bool anchorOk = qAbs(sourceXBefore - sourceXAfter) < 0.01
        && qAbs(sourceYBefore - sourceYAfter) < 0.01;
    sendWheel(pin, zoomAnchorGlobal, -120);
    sendWheel(pin, zoomAnchorGlobal, -120);
    const bool restoreOk = qAbs(pin.width() - originalSize.width()) <= 2
        && qAbs(pin.height() - originalSize.height()) <= 2;
    const bool zoomOk = firstZoomOk && cumulativeZoomOk && anchorOk && restoreOk;
    ok &= zoomOk;

    QImage hidpiImage(640, 400, QImage::Format_ARGB32_Premultiplied);
    hidpiImage.fill(QColor(200, 120, 40));
    hidpiImage.setDevicePixelRatio(2.0);
    snipnexs::PinWindow hidpiPin(hidpiImage);
    hidpiPin.show();
    QApplication::processEvents();
    const bool hidpiSizeOk = hidpiPin.size() == QSize(320 + kMarginTotal, 200 + kMarginTotal);
    ok &= hidpiSizeOk;

    QImage smallImage(30, 20, QImage::Format_ARGB32_Premultiplied);
    smallImage.fill(QColor(90, 90, 220));
    snipnexs::PinWindow smallPin(smallImage);
    smallPin.show();
    QApplication::processEvents();
    const bool smallSizeOk = smallPin.size() == QSize(30 + kMarginTotal, 20 + kMarginTotal);
    ok &= smallSizeOk;

    const QSize largeImageSize = QGuiApplication::primaryScreen()->availableGeometry().size();
    QImage largeImage(largeImageSize, QImage::Format_ARGB32_Premultiplied);
    largeImage.fill(QColor(45, 80, 120));
    snipnexs::PinWindow largePin(largeImage);
    largePin.show();
    QApplication::processEvents();
    const bool largeSizeOk = largePin.size() == largeImageSize + QSize(kMarginTotal, kMarginTotal);
    ok &= largeSizeOk;

    sendWheel(pin, pin.mapToGlobal(pin.rect().center()), 120);
    const QImage grabImage = pin.grab().toImage().convertToFormat(QImage::Format_ARGB32);
    QColor shadowPixel(0, 0, 0, 0);
    for (const QPoint& probe : { QPoint(8, 8), QPoint(11, 11), QPoint(13, 13), QPoint(6, 6), QPoint(3, 3) }) {
        shadowPixel = grabImage.pixelColor(probe);
        QTextStream(stdout) << "probe " << probe.x() << ',' << probe.y()
                            << " alpha=" << shadowPixel.alpha() << '\n';
        if (shadowPixel.alpha() > 0) {
            break;
        }
    }
    const QColor imagePixel = grabImage.pixelColor(grabImage.rect().center());
    const QColor imageEdgePixel = grabImage.pixelColor(
        grabImage.width() - kMarginTotal / 2 - 2,
        grabImage.height() / 2);
    const bool shadowOk = shadowPixel.alpha() > 0 && shadowPixel.alpha() <= 24;
    const bool imageOk = imagePixel == QColor(40, 160, 200)
        && imageEdgePixel == QColor(40, 160, 200);
    ok &= shadowOk;
    ok &= imageOk;

    QTextStream(stdout)
        << "pin window: " << (ok ? "ok" : "failed")
        << " initial=" << initialSizeOk
        << " imagePosition=" << imagePositionOk
        << " drag=" << dragOk
        << " zoom=" << zoomOk
        << " cumulative=" << cumulativeZoomOk
        << " anchor=" << anchorOk
        << " restore=" << restoreOk
        << " hidpi=" << hidpiSizeOk
        << " small=" << smallSizeOk
        << " large=" << largeSizeOk
        << " shadow=" << shadowOk
        << " image=" << imageOk
        << " position=" << pin.x() << ',' << pin.y()
        << " size=" << pin.width() << 'x' << pin.height() << '\n';
    return ok ? 0 : 1;
}
