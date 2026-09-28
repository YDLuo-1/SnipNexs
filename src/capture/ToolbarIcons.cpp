#include "capture/ToolbarIcons.h"

#include <QPainter>
#include <QPainterPath>

#include <cstddef>

// Glyph geometry adopted from the Lucide Icons library, https://lucide.dev,
// version 1.48.0 (ISC license, see licenses/Lucide-ISC.txt). The 24x24 SVG
// path data was converted to these segment tables; rendering scales it 2x
// onto the 48x48 icon canvas with the library's 2 px round stroke.

namespace snipnexs {

namespace {

struct Seg {
    enum Kind { Move, Line, Cubic, Close } kind;
    float p[6];
};

struct Glyph {
    const Seg* segs;
    std::size_t count;
};

const Seg kPenSegs[] = {
    {Seg::Move, {21.2f, 6.81f}},
    {Seg::Cubic, {22.3f, 5.71f, 22.3f, 3.93f, 21.2f, 2.83f}},
    {Seg::Cubic, {20.1f, 1.72f, 18.3f, 1.72f, 17.2f, 2.83f}},
    {Seg::Line, {3.84f, 16.2f}},
    {Seg::Cubic, {3.61f, 16.4f, 3.44f, 16.7f, 3.34f, 17.0f}},
    {Seg::Line, {2.02f, 21.4f}},
    {Seg::Cubic, {1.97f, 21.5f, 2.02f, 21.7f, 2.15f, 21.9f}},
    {Seg::Cubic, {2.28f, 22.0f, 2.47f, 22.0f, 2.64f, 22.0f}},
    {Seg::Line, {7.0f, 20.7f}},
    {Seg::Cubic, {7.31f, 20.6f, 7.6f, 20.4f, 7.83f, 20.2f}},
    {Seg::Close, {}},
    {Seg::Move, {15.0f, 5.0f}},
    {Seg::Line, {19.0f, 9.0f}},
};
const Seg kRectangleSegs[] = {
    {Seg::Move, {5.0f, 3.0f}},
    {Seg::Line, {19.0f, 3.0f}},
    {Seg::Cubic, {20.1f, 3.0f, 21.0f, 3.9f, 21.0f, 5.0f}},
    {Seg::Line, {21.0f, 19.0f}},
    {Seg::Cubic, {21.0f, 20.1f, 20.1f, 21.0f, 19.0f, 21.0f}},
    {Seg::Line, {5.0f, 21.0f}},
    {Seg::Cubic, {3.9f, 21.0f, 3.0f, 20.1f, 3.0f, 19.0f}},
    {Seg::Line, {3.0f, 5.0f}},
    {Seg::Cubic, {3.0f, 3.9f, 3.9f, 3.0f, 5.0f, 3.0f}},
    {Seg::Close, {}},
};
const Seg kArrowSegs[] = {
    {Seg::Move, {7.0f, 7.0f}},
    {Seg::Line, {17.0f, 7.0f}},
    {Seg::Line, {17.0f, 17.0f}},
    {Seg::Move, {7.0f, 17.0f}},
    {Seg::Line, {17.0f, 7.0f}},
};
const Seg kTextSegs[] = {
    {Seg::Move, {17.0f, 22.0f}},
    {Seg::Line, {16.0f, 22.0f}},
    {Seg::Cubic, {13.8f, 22.0f, 12.0f, 20.2f, 12.0f, 18.0f}},
    {Seg::Line, {12.0f, 6.0f}},
    {Seg::Cubic, {12.0f, 3.79f, 13.8f, 2.0f, 16.0f, 2.0f}},
    {Seg::Line, {17.0f, 2.0f}},
    {Seg::Move, {7.0f, 22.0f}},
    {Seg::Line, {8.0f, 22.0f}},
    {Seg::Cubic, {10.2f, 22.0f, 12.0f, 20.2f, 12.0f, 18.0f}},
    {Seg::Move, {7.0f, 2.0f}},
    {Seg::Line, {8.0f, 2.0f}},
    {Seg::Cubic, {10.2f, 2.0f, 12.0f, 3.79f, 12.0f, 6.0f}},
};
const Seg kColorPickerSegs[] = {
    {Seg::Move, {12.0f, 9.0f}},
    {Seg::Line, {3.59f, 17.4f}},
    {Seg::Cubic, {3.21f, 17.8f, 3.0f, 18.3f, 3.0f, 18.8f}},
    {Seg::Line, {3.0f, 20.2f}},
    {Seg::Cubic, {3.0f, 20.7f, 2.79f, 21.2f, 2.41f, 21.6f}},
    {Seg::Cubic, {2.79f, 21.2f, 3.3f, 21.0f, 3.83f, 21.0f}},
    {Seg::Line, {5.17f, 21.0f}},
    {Seg::Cubic, {5.7f, 21.0f, 6.21f, 20.8f, 6.59f, 20.4f}},
    {Seg::Line, {15.0f, 12.0f}},
    {Seg::Move, {18.0f, 9.0f}},
    {Seg::Line, {18.4f, 9.4f}},
    {Seg::Cubic, {19.2f, 10.2f, 19.2f, 11.6f, 18.4f, 12.4f}},
    {Seg::Cubic, {17.6f, 13.2f, 16.2f, 13.2f, 15.4f, 12.4f}},
    {Seg::Line, {11.6f, 8.6f}},
    {Seg::Cubic, {10.8f, 7.77f, 10.8f, 6.43f, 11.6f, 5.6f}},
    {Seg::Cubic, {12.4f, 4.77f, 13.8f, 4.77f, 14.6f, 5.6f}},
    {Seg::Line, {15.0f, 6.0f}},
    {Seg::Line, {18.4f, 2.6f}},
    {Seg::Cubic, {18.9f, 2.06f, 19.7f, 1.85f, 20.4f, 2.05f}},
    {Seg::Cubic, {21.2f, 2.25f, 21.8f, 2.82f, 21.9f, 3.55f}},
    {Seg::Cubic, {22.1f, 4.28f, 21.9f, 5.06f, 21.4f, 5.6f}},
    {Seg::Close, {}},
    {Seg::Move, {2.0f, 22.0f}},
    {Seg::Line, {2.41f, 21.6f}},
};
const Seg kUndoSegs[] = {
    {Seg::Move, {9.0f, 14.0f}},
    {Seg::Line, {4.0f, 9.0f}},
    {Seg::Line, {9.0f, 4.0f}},
    {Seg::Move, {4.0f, 9.0f}},
    {Seg::Line, {14.5f, 9.0f}},
    {Seg::Cubic, {17.5f, 9.0f, 20.0f, 11.5f, 20.0f, 14.5f}},
    {Seg::Cubic, {20.0f, 17.5f, 17.5f, 20.0f, 14.5f, 20.0f}},
    {Seg::Line, {11.0f, 20.0f}},
};
const Seg kRedoSegs[] = {
    {Seg::Move, {15.0f, 14.0f}},
    {Seg::Line, {20.0f, 9.0f}},
    {Seg::Line, {15.0f, 4.0f}},
    {Seg::Move, {20.0f, 9.0f}},
    {Seg::Line, {9.5f, 9.0f}},
    {Seg::Cubic, {6.46f, 9.0f, 4.0f, 11.5f, 4.0f, 14.5f}},
    {Seg::Cubic, {4.0f, 17.5f, 6.46f, 20.0f, 9.5f, 20.0f}},
    {Seg::Line, {13.0f, 20.0f}},
};
const Seg kOcrSegs[] = {
    {Seg::Move, {3.0f, 7.0f}},
    {Seg::Line, {3.0f, 5.0f}},
    {Seg::Cubic, {3.0f, 3.9f, 3.9f, 3.0f, 5.0f, 3.0f}},
    {Seg::Line, {7.0f, 3.0f}},
    {Seg::Move, {17.0f, 3.0f}},
    {Seg::Line, {19.0f, 3.0f}},
    {Seg::Cubic, {20.1f, 3.0f, 21.0f, 3.9f, 21.0f, 5.0f}},
    {Seg::Line, {21.0f, 7.0f}},
    {Seg::Move, {21.0f, 17.0f}},
    {Seg::Line, {21.0f, 19.0f}},
    {Seg::Cubic, {21.0f, 20.1f, 20.1f, 21.0f, 19.0f, 21.0f}},
    {Seg::Line, {17.0f, 21.0f}},
    {Seg::Move, {7.0f, 21.0f}},
    {Seg::Line, {5.0f, 21.0f}},
    {Seg::Cubic, {3.9f, 21.0f, 3.0f, 20.1f, 3.0f, 19.0f}},
    {Seg::Line, {3.0f, 17.0f}},
    {Seg::Move, {7.0f, 8.0f}},
    {Seg::Line, {15.0f, 8.0f}},
    {Seg::Move, {7.0f, 12.0f}},
    {Seg::Line, {17.0f, 12.0f}},
    {Seg::Move, {7.0f, 16.0f}},
    {Seg::Line, {13.0f, 16.0f}},
};
const Seg kPinSegs[] = {
    {Seg::Move, {12.0f, 17.0f}},
    {Seg::Line, {12.0f, 22.0f}},
    {Seg::Move, {9.0f, 10.8f}},
    {Seg::Cubic, {9.0f, 11.5f, 8.57f, 12.2f, 7.89f, 12.5f}},
    {Seg::Line, {6.11f, 13.5f}},
    {Seg::Cubic, {5.43f, 13.8f, 5.0f, 14.5f, 5.0f, 15.2f}},
    {Seg::Line, {5.0f, 16.0f}},
    {Seg::Cubic, {5.0f, 16.6f, 5.45f, 17.0f, 6.0f, 17.0f}},
    {Seg::Line, {18.0f, 17.0f}},
    {Seg::Cubic, {18.6f, 17.0f, 19.0f, 16.6f, 19.0f, 16.0f}},
    {Seg::Line, {19.0f, 15.2f}},
    {Seg::Cubic, {19.0f, 14.5f, 18.6f, 13.8f, 17.9f, 13.5f}},
    {Seg::Line, {16.1f, 12.5f}},
    {Seg::Cubic, {15.4f, 12.2f, 15.0f, 11.5f, 15.0f, 10.8f}},
    {Seg::Line, {15.0f, 7.0f}},
    {Seg::Cubic, {15.0f, 6.45f, 15.4f, 6.0f, 16.0f, 6.0f}},
    {Seg::Cubic, {17.1f, 6.0f, 18.0f, 5.1f, 18.0f, 4.0f}},
    {Seg::Cubic, {18.0f, 2.9f, 17.1f, 2.0f, 16.0f, 2.0f}},
    {Seg::Line, {8.0f, 2.0f}},
    {Seg::Cubic, {6.9f, 2.0f, 6.0f, 2.9f, 6.0f, 4.0f}},
    {Seg::Cubic, {6.0f, 5.1f, 6.9f, 6.0f, 8.0f, 6.0f}},
    {Seg::Cubic, {8.55f, 6.0f, 9.0f, 6.45f, 9.0f, 7.0f}},
    {Seg::Close, {}},
};
const Seg kRecordSegs[] = {
    {Seg::Move, {16.0f, 13.0f}},
    {Seg::Line, {21.2f, 16.5f}},
    {Seg::Cubic, {21.4f, 16.6f, 21.6f, 16.6f, 21.7f, 16.5f}},
    {Seg::Cubic, {21.9f, 16.4f, 22.0f, 16.3f, 22.0f, 16.1f}},
    {Seg::Line, {22.0f, 7.87f}},
    {Seg::Cubic, {22.0f, 7.69f, 21.9f, 7.53f, 21.7f, 7.44f}},
    {Seg::Cubic, {21.6f, 7.35f, 21.4f, 7.35f, 21.2f, 7.44f}},
    {Seg::Line, {16.0f, 10.5f}},
    {Seg::Move, {4.0f, 6.0f}},
    {Seg::Line, {14.0f, 6.0f}},
    {Seg::Cubic, {15.1f, 6.0f, 16.0f, 6.9f, 16.0f, 8.0f}},
    {Seg::Line, {16.0f, 16.0f}},
    {Seg::Cubic, {16.0f, 17.1f, 15.1f, 18.0f, 14.0f, 18.0f}},
    {Seg::Line, {4.0f, 18.0f}},
    {Seg::Cubic, {2.9f, 18.0f, 2.0f, 17.1f, 2.0f, 16.0f}},
    {Seg::Line, {2.0f, 8.0f}},
    {Seg::Cubic, {2.0f, 6.9f, 2.9f, 6.0f, 4.0f, 6.0f}},
    {Seg::Close, {}},
};
const Seg kCopySegs[] = {
    {Seg::Move, {10.0f, 8.0f}},
    {Seg::Line, {20.0f, 8.0f}},
    {Seg::Cubic, {21.1f, 8.0f, 22.0f, 8.9f, 22.0f, 10.0f}},
    {Seg::Line, {22.0f, 20.0f}},
    {Seg::Cubic, {22.0f, 21.1f, 21.1f, 22.0f, 20.0f, 22.0f}},
    {Seg::Line, {10.0f, 22.0f}},
    {Seg::Cubic, {8.9f, 22.0f, 8.0f, 21.1f, 8.0f, 20.0f}},
    {Seg::Line, {8.0f, 10.0f}},
    {Seg::Cubic, {8.0f, 8.9f, 8.9f, 8.0f, 10.0f, 8.0f}},
    {Seg::Close, {}},
    {Seg::Move, {4.0f, 16.0f}},
    {Seg::Cubic, {2.9f, 16.0f, 2.0f, 15.1f, 2.0f, 14.0f}},
    {Seg::Line, {2.0f, 4.0f}},
    {Seg::Cubic, {2.0f, 2.9f, 2.9f, 2.0f, 4.0f, 2.0f}},
    {Seg::Line, {14.0f, 2.0f}},
    {Seg::Cubic, {15.1f, 2.0f, 16.0f, 2.9f, 16.0f, 4.0f}},
};
const Seg kSaveSegs[] = {
    {Seg::Move, {15.2f, 3.0f}},
    {Seg::Cubic, {15.7f, 3.01f, 16.2f, 3.22f, 16.6f, 3.6f}},
    {Seg::Line, {20.4f, 7.4f}},
    {Seg::Cubic, {20.8f, 7.77f, 21.0f, 8.27f, 21.0f, 8.8f}},
    {Seg::Line, {21.0f, 19.0f}},
    {Seg::Cubic, {21.0f, 20.1f, 20.1f, 21.0f, 19.0f, 21.0f}},
    {Seg::Line, {5.0f, 21.0f}},
    {Seg::Cubic, {3.9f, 21.0f, 3.0f, 20.1f, 3.0f, 19.0f}},
    {Seg::Line, {3.0f, 5.0f}},
    {Seg::Cubic, {3.0f, 3.9f, 3.9f, 3.0f, 5.0f, 3.0f}},
    {Seg::Close, {}},
    {Seg::Move, {17.0f, 21.0f}},
    {Seg::Line, {17.0f, 14.0f}},
    {Seg::Cubic, {17.0f, 13.4f, 16.6f, 13.0f, 16.0f, 13.0f}},
    {Seg::Line, {8.0f, 13.0f}},
    {Seg::Cubic, {7.45f, 13.0f, 7.0f, 13.4f, 7.0f, 14.0f}},
    {Seg::Line, {7.0f, 21.0f}},
    {Seg::Move, {7.0f, 3.0f}},
    {Seg::Line, {7.0f, 7.0f}},
    {Seg::Cubic, {7.0f, 7.55f, 7.45f, 8.0f, 8.0f, 8.0f}},
    {Seg::Line, {15.0f, 8.0f}},
};
const Seg kCancelSegs[] = {
    {Seg::Move, {18.0f, 6.0f}},
    {Seg::Line, {6.0f, 18.0f}},
    {Seg::Move, {6.0f, 6.0f}},
    {Seg::Line, {18.0f, 18.0f}},
};
const Seg kCheckSegs[] = {
    {Seg::Move, {20.0f, 6.0f}},
    {Seg::Line, {9.0f, 17.0f}},
    {Seg::Line, {4.0f, 12.0f}},
};

Glyph glyphFor(ToolbarIcon icon)
{
    switch (icon) {
    case ToolbarIcon::Pen: return {kPenSegs, 13};
    case ToolbarIcon::Rectangle: return {kRectangleSegs, 10};
    case ToolbarIcon::Arrow: return {kArrowSegs, 5};
    case ToolbarIcon::Text: return {kTextSegs, 12};
    case ToolbarIcon::ColorPicker: return {kColorPickerSegs, 24};
    case ToolbarIcon::Undo: return {kUndoSegs, 8};
    case ToolbarIcon::Redo: return {kRedoSegs, 8};
    case ToolbarIcon::Ocr: return {kOcrSegs, 22};
    case ToolbarIcon::Pin: return {kPinSegs, 23};
    case ToolbarIcon::Record: return {kRecordSegs, 18};
    case ToolbarIcon::Copy: return {kCopySegs, 16};
    case ToolbarIcon::Save: return {kSaveSegs, 21};
    case ToolbarIcon::Cancel: return {kCancelSegs, 4};
    case ToolbarIcon::Check: return {kCheckSegs, 3};
    }
    return {nullptr, 0};
}

} // namespace

QPixmap drawToolbarIcon(ToolbarIcon icon, const QColor& color)
{
    QPixmap pixmap(48, 48);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(2.0, 2.0);
    painter.setPen(QPen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    const Glyph glyph = glyphFor(icon);
    QPainterPath path;
    for (std::size_t i = 0; i < glyph.count; ++i) {
        const Seg& segment = glyph.segs[i];
        switch (segment.kind) {
        case Seg::Move:
            path.moveTo(segment.p[0], segment.p[1]);
            break;
        case Seg::Line:
            path.lineTo(segment.p[0], segment.p[1]);
            break;
        case Seg::Cubic:
            path.cubicTo(segment.p[0], segment.p[1], segment.p[2],
                         segment.p[3], segment.p[4], segment.p[5]);
            break;
        case Seg::Close:
            path.closeSubpath();
            break;
        }
    }
    painter.drawPath(path);
    painter.end();
    return pixmap;
}

QIcon makeToolbarIcon(ToolbarIcon icon, bool onDarkBackground)
{
    QIcon result;
    // The confirm check is the one accent-colored action in the set.
    const bool accent = icon == ToolbarIcon::Check && !onDarkBackground;
    const QColor normal = accent
        ? QColor(35, 139, 218)
        : (onDarkBackground ? QColor(240, 244, 249) : QColor(53, 65, 76));
    const QColor active = accent
        ? QColor(24, 104, 165)
        : (onDarkBackground ? QColor(255, 255, 255) : QColor(20, 29, 37));
    const QColor disabled = onDarkBackground
        ? QColor(240, 244, 249, 90)
        : QColor(158, 168, 177);
    result.addPixmap(drawToolbarIcon(icon, normal), QIcon::Normal, QIcon::Off);
    result.addPixmap(drawToolbarIcon(icon, active), QIcon::Active, QIcon::Off);
    result.addPixmap(drawToolbarIcon(icon, disabled), QIcon::Disabled, QIcon::Off);
    result.addPixmap(drawToolbarIcon(icon, QColor(255, 255, 255)), QIcon::Normal, QIcon::On);
    result.addPixmap(drawToolbarIcon(icon, QColor(255, 255, 255)), QIcon::Active, QIcon::On);
    return result;
}

} // namespace snipnexs
