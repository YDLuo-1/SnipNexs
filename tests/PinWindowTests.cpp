#include "pin/PinWindow.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QFrame>
#include <QImage>
#include <QMenu>
#include <QPainter>
#include <QPointingDevice>
#include <QPointer>
#include <QSignalSpy>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>
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

bool openMenuAndClose(snipnexs::PinWindow& pin)
{
    bool menuSeen = false;
    QTimer::singleShot(0, [&]() {
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            if (auto* menu = qobject_cast<QMenu*>(widget)) {
                menuSeen = true;
                menu->close();
                break;
            }
        }
    });
    const QPoint local = pin.rect().center();
    QContextMenuEvent event(
        QContextMenuEvent::Mouse, local, pin.mapToGlobal(local));
    QApplication::sendEvent(&pin, &event);
    QApplication::processEvents();
    return menuSeen;
}

bool openMenuAndTrigger(snipnexs::PinWindow& pin, int actionIndex, bool& actionCountOk)
{
    bool menuSeen = false;
    QTimer::singleShot(0, [&]() {
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            auto* menu = qobject_cast<QMenu*>(widget);
            if (menu == nullptr) {
                continue;
            }
            menuSeen = true;
            actionCountOk = menu->actions().size() >= 5;
            if (actionIndex >= 0 && actionIndex < menu->actions().size()) {
                menu->actions().at(actionIndex)->trigger();
            } else {
                menu->close();
            }
            break;
        }
    });
    const QPoint local = pin.rect().center();
    QContextMenuEvent event(
        QContextMenuEvent::Mouse, local, pin.mapToGlobal(local));
    QApplication::sendEvent(&pin, &event);
    QApplication::processEvents();
    return menuSeen;
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

    QImage quadrantImage(640, 400, QImage::Format_ARGB32_Premultiplied);
    quadrantImage.fill(Qt::black);
    QPainter quadrantPainter(&quadrantImage);
    quadrantPainter.fillRect(QRect(0, 0, 320, 200), QColor(220, 40, 40));
    quadrantPainter.fillRect(QRect(320, 0, 320, 200), QColor(40, 180, 70));
    quadrantPainter.fillRect(QRect(0, 200, 320, 200), QColor(50, 90, 220));
    quadrantPainter.fillRect(QRect(320, 200, 320, 200), QColor(220, 180, 40));
    quadrantPainter.end();
    quadrantImage.setDevicePixelRatio(2.0);
    snipnexs::PinWindow quadrantPin(quadrantImage);
    quadrantPin.show();
    QApplication::processEvents();
    const QImage quadrantGrab = quadrantPin.grab().toImage().convertToFormat(
        QImage::Format_ARGB32);
    const int imageLeft = kMarginTotal / 2;
    const int imageTop = kMarginTotal / 2;
    const int imageWidth = quadrantGrab.width() - kMarginTotal;
    const int imageHeight = quadrantGrab.height() - kMarginTotal;
    const bool quadrantOk = quadrantGrab.pixelColor(
            imageLeft + imageWidth / 4, imageTop + imageHeight / 4)
            == QColor(220, 40, 40)
        && quadrantGrab.pixelColor(
               imageLeft + imageWidth * 3 / 4, imageTop + imageHeight / 4)
            == QColor(40, 180, 70)
        && quadrantGrab.pixelColor(
               imageLeft + imageWidth / 4, imageTop + imageHeight * 3 / 4)
            == QColor(50, 90, 220)
        && quadrantGrab.pixelColor(
               imageLeft + imageWidth * 3 / 4, imageTop + imageHeight * 3 / 4)
            == QColor(220, 180, 40);
    ok &= quadrantOk;

    QSignalSpy copySpy(&quadrantPin, &snipnexs::PinWindow::copyRequested);
    auto* copyButton = quadrantPin.findChild<QToolButton*>(
        QStringLiteral("pinCopyButton"));
    if (copyButton != nullptr) {
        copyButton->click();
    }
    QImage copiedImage;
    if (copySpy.size() == 1) {
        copiedImage = qvariant_cast<QImage>(copySpy.at(0).at(0));
    }
    const bool originalSourceOk = copyButton != nullptr
        && copySpy.size() == 1
        && copiedImage.size() == quadrantImage.size()
        && qAbs(copiedImage.devicePixelRatio() - quadrantImage.devicePixelRatio()) < 0.001;
    ok &= originalSourceOk;

    const bool rightMenuSeen = openMenuAndClose(quadrantPin);
    const bool rightClickKeepsPin = quadrantPin.isVisible();
    bool menuActionsOk = false;
    const bool copyMenuSeen = openMenuAndTrigger(quadrantPin, 0, menuActionsOk);
    const bool toolbarMenuSeen = openMenuAndTrigger(quadrantPin, 2, menuActionsOk);
    const auto* toolbar = quadrantPin.findChild<QFrame*>(QStringLiteral("pinToolbar"));
    const bool toolbarToggleOk = toolbar != nullptr && toolbar->isVisible();
    ok &= rightMenuSeen && rightClickKeepsPin && copyMenuSeen && toolbarMenuSeen
        && menuActionsOk && toolbarToggleOk;

    QPointer<snipnexs::PinWindow> doubleClickPin = new snipnexs::PinWindow(image);
    doubleClickPin->show();
    QApplication::processEvents();
    QTest::mouseDClick(
        doubleClickPin, Qt::LeftButton, Qt::NoModifier, doubleClickPin->rect().center());
    QApplication::processEvents();
    const bool doubleClickCloseOk = doubleClickPin == nullptr
        || !doubleClickPin->isVisible();
    ok &= doubleClickCloseOk;

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
        << " quadrants=" << quadrantOk
        << " originalSource=" << originalSourceOk
        << " rightMenu=" << (rightMenuSeen && rightClickKeepsPin)
        << " menuActions=" << (copyMenuSeen && toolbarMenuSeen && menuActionsOk)
        << " toolbar=" << toolbarToggleOk
        << " doubleClickClose=" << doubleClickCloseOk
        << " small=" << smallSizeOk
        << " large=" << largeSizeOk
        << " shadow=" << shadowOk
        << " image=" << imageOk
        << " position=" << pin.x() << ',' << pin.y()
        << " size=" << pin.width() << 'x' << pin.height() << '\n';
    return ok ? 0 : 1;
}
