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
};

// Renders a glyph on a 48x48 canvas with a uniform 3.4 px round stroke.
[[nodiscard]] QPixmap drawToolbarIcon(ToolbarIcon icon, const QColor& color);

// Builds the four-state icon used by toolbar buttons. Set onDarkBackground
// for the pin toolbar (light glyphs on translucent dark chrome).
[[nodiscard]] QIcon makeToolbarIcon(ToolbarIcon icon, bool onDarkBackground = false);

} // namespace snipnexs
