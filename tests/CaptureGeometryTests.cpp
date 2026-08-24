#include "capture/CaptureGeometry.h"

#include <QTextStream>

namespace {

bool expectEqual(const QRect& actual, const QRect& expected, const char* name)
{
    if (actual == expected) {
        return true;
    }
    QTextStream(stderr) << name << " failed: actual=("
                        << actual.x() << ',' << actual.y() << ','
                        << actual.width() << ',' << actual.height() << "), expected=("
                        << expected.x() << ',' << expected.y() << ','
                        << expected.width() << ',' << expected.height() << ")\n";
    return false;
}

} // namespace

int main()
{
    bool ok = true;
    ok &= expectEqual(
        snipnexs::logicalToPixelRect(QRect(10, 20, 100, 50), 1.5, QSize(1920, 1080)),
        QRect(15, 30, 150, 75),
        "scaled selection");
    ok &= expectEqual(
        snipnexs::logicalToPixelRect(QRect(-5, -4, 20, 14), 2.0, QSize(100, 100)),
        QRect(0, 0, 30, 20),
        "clipped selection");
    ok &= expectEqual(
        snipnexs::logicalToPixelRect(QRect(), 1.0, QSize(100, 100)),
        QRect(),
        "empty selection");
    ok &= expectEqual(
        snipnexs::nativeWindowToLogicalRect(
            QRect(2070, 150, 900, 600),
            QRect(1920, 0, 1920, 1080),
            QSize(1280, 720)),
        QRect(100, 100, 600, 400),
        "window target at 150 percent");
    ok &= expectEqual(
        snipnexs::nativeWindowToLogicalRect(
            QRect(-300, -150, 900, 600),
            QRect(0, 0, 1920, 1080),
            QSize(1280, 720)),
        QRect(0, 0, 400, 300),
        "window target clipped to monitor");

    QTextStream(stdout) << (ok ? "capture geometry: ok\n" : "capture geometry: failed\n");
    return ok ? 0 : 1;
}
