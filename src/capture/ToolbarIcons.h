#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>

namespace snipnexs {

enum class ToolbarIcon {
    Pen,
    Rectangle,
    Arrow,
    Text,
    ColorPicker,
    Undo,
    Redo,
    Ocr,
    Pin,
    Record,
    Copy,
    Save,
    Cancel,
    Check,
};

// Renders a glyph on a 48x48 canvas with a uniform 4 px round stroke
// (24-grid scaled 2x). Kept for tests and generic consumers.
[[nodiscard]] QPixmap drawToolbarIcon(ToolbarIcon icon, const QColor& color);

// Renders a glyph for a specific logical size and device pixel ratio so the
// stroke lands on real device pixels instead of being scaled from a bitmap
// (crisp on 100%/125%/150%/200% displays).
[[nodiscard]] QPixmap drawToolbarIconAt(
    ToolbarIcon icon, const QColor& color, int logicalSize, qreal devicePixelRatio);

// Builds the four-state icon used by toolbar buttons. Set onDarkBackground
// for the pin toolbar (light glyphs on translucent dark chrome).
[[nodiscard]] QIcon makeToolbarIcon(ToolbarIcon icon, bool onDarkBackground = false);

} // namespace snipnexs
