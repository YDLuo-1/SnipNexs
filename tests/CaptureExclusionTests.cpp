#include "platform/windows/CaptureExclusion.h"

#include <QApplication>
#include <QColor>
#include <QGuiApplication>
#include <QPalette>
#include <QPixmap>
#include <QScreen>
#include <QTextStream>
#include <QThread>
#include <QWidget>

#include <dwmapi.h>

namespace {

bool nearColor(const QColor& actual, const QColor& expected)
{
    constexpr int tolerance = 8;
    return qAbs(actual.red() - expected.red()) <= tolerance
        && qAbs(actual.green() - expected.green()) <= tolerance
        && qAbs(actual.blue() - expected.blue()) <= tolerance;
}

QColor grabCenterPixel(QScreen& screen, const QRect& logicalRect)
{
    const QRect localRect = logicalRect.translated(-screen.geometry().topLeft());
    const QPixmap grabbed = screen.grabWindow(
        0, localRect.x(), localRect.y(), localRect.width(), localRect.height());
    const QImage image = grabbed.toImage();
    return image.isNull()
        ? QColor()
        : image.pixelColor(image.width() / 2, image.height() / 2);
}

void setSolidColor(QWidget& widget, const QColor& color)
{
    widget.setAutoFillBackground(true);
    QPalette palette = widget.palette();
    palette.setColor(QPalette::Window, color);
    widget.setPalette(palette);
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen == nullptr) {
        QTextStream(stdout) << "capture exclusion: no screen\n";
        return 1;
    }

    const QColor backgroundColor(24, 164, 204);
    const QColor foregroundColor(232, 48, 112);
    const QRect available = screen->availableGeometry();
    const QRect backgroundRect(available.topLeft() + QPoint(32, 32), QSize(280, 180));
    const QRect foregroundRect(backgroundRect.topLeft() + QPoint(60, 40), QSize(150, 90));

    QWidget background(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    QWidget foreground(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setSolidColor(background, backgroundColor);
    setSolidColor(foreground, foregroundColor);
    background.setGeometry(backgroundRect);
    foreground.setGeometry(foregroundRect);
    background.show();
    foreground.show();
    foreground.raise();
    QApplication::processEvents();
    QThread::msleep(80);
    DwmFlush();

    const QColor before = grabCenterPixel(*screen, foregroundRect);
    const bool excluded = snipnexs::setWindowExcludedFromCapture(foreground, true);
    QApplication::processEvents();
    DwmFlush();
    QThread::msleep(80);
    const QColor after = grabCenterPixel(*screen, foregroundRect);
    const bool reset = snipnexs::setWindowExcludedFromCapture(foreground, false);
    Q_UNUSED(reset);

    if (excluded && before == QColor(Qt::black) && after == QColor(Qt::black)) {
        QTextStream(stdout)
            << "capture exclusion: skipped (desktop capture returned black frames)\n";
        return 77;
    }

    const bool ok = excluded
        && nearColor(before, foregroundColor)
        && nearColor(after, backgroundColor);
    QTextStream(stdout)
        << "capture exclusion: " << (ok ? "ok" : "failed")
        << " before=" << before.name()
        << " after=" << after.name() << '\n';
    return ok ? 0 : 1;
}
