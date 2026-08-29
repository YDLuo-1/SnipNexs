#include "capture/ToolbarIcons.h"

#include <QGuiApplication>
#include <QImage>
#include <QTextStream>

#include <cstring>
#include <vector>

namespace {

bool hasVisibleInk(const QPixmap& pixmap)
{
    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    int inked = 0;
    for (int y = 0; y < image.height(); ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(line[x]) > 40) {
                ++inked;
            }
        }
    }
    // Glyphs occupy roughly 4-12% of the 48x48 canvas once strokes are
    // anti-aliased; a broken glyph renders blank or a full-canvas blob.
    return inked > 120 && inked < 48 * 48 / 2;
}

bool differsFrom(const QPixmap& left, const QPixmap& right)
{
    const QImage a = left.toImage().convertToFormat(QImage::Format_ARGB32);
    const QImage b = right.toImage().convertToFormat(QImage::Format_ARGB32);
    if (a.size() != b.size()) {
        return true;
    }
    for (int y = 0; y < a.height(); ++y) {
        if (std::memcmp(a.constScanLine(y), b.constScanLine(y),
                static_cast<size_t>(a.bytesPerLine())) != 0) {
            return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    const std::vector<snipnexs::ToolbarIcon> icons = {
        snipnexs::ToolbarIcon::Pen, snipnexs::ToolbarIcon::Rectangle,
        snipnexs::ToolbarIcon::Arrow, snipnexs::ToolbarIcon::Text,
        snipnexs::ToolbarIcon::ColorPicker, snipnexs::ToolbarIcon::Undo,
        snipnexs::ToolbarIcon::Redo, snipnexs::ToolbarIcon::Ocr,
        snipnexs::ToolbarIcon::Pin, snipnexs::ToolbarIcon::Record,
        snipnexs::ToolbarIcon::Copy, snipnexs::ToolbarIcon::Save,
        snipnexs::ToolbarIcon::Cancel, snipnexs::ToolbarIcon::Check,
    };

    bool ok = true;
    QTextStream stream(stderr);
    for (const snipnexs::ToolbarIcon icon : icons) {
        const QPixmap normal = snipnexs::drawToolbarIcon(
            icon, QColor(53, 65, 76));
        if (normal.size() != QSize(48, 48) || !hasVisibleInk(normal)) {
            stream << "icon " << static_cast<int>(icon)
                   << " has no usable ink\n";
            ok = false;
        }
        for (const auto other : icons) {
            if (other >= icon) {
                continue;
            }
            if (!differsFrom(normal, snipnexs::drawToolbarIcon(
                    other, QColor(53, 65, 76)))) {
                stream << "icons " << static_cast<int>(other) << " and "
                       << static_cast<int>(icon) << " render identically\n";
                ok = false;
            }
        }
    }

    // Both toolbar themes must produce four-state icons.
    const QIcon light = snipnexs::makeToolbarIcon(snipnexs::ToolbarIcon::Pin);
    const QIcon dark = snipnexs::makeToolbarIcon(
        snipnexs::ToolbarIcon::Pin, true);
    ok &= !light.availableSizes().isEmpty();
    ok &= !dark.availableSizes().isEmpty();

    QTextStream(stdout) << "toolbar-icons-tests: " << (ok ? "ok" : "failed")
                        << '\n';
    return ok ? 0 : 1;
}
