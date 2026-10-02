"""Generates the parts-based ToolbarIcons.cpp from Material Symbols Rounded.

Material SVGs draw filled shapes (no fill attribute = solid per SVG spec),
so parse_svg must treat missing fill as filled for these sources. The
emitted tables carry per-part fill flags; the renderer fills or strokes
each part accordingly.
"""
import io
import os
import sys

sys.path.insert(0, r"D:\CodexProjects\ScreenBox\build\iconwork")
import gen_icons

MATERIAL_DIR = r"D:\CodexProjects\ScreenBox\build\iconwork\material"

# slot -> (material file, licence note in commit message only)
FINAL = {
    "Pen": "edit-fill1.svg",
    "Rectangle": "crop_square-default.svg",
    "Arrow": "arrow_outward-default.svg",
    "Text": "title-fill1.svg",
    "ColorPicker": "colorize-fill1.svg",
    "Undo": "undo-default.svg",
    "Redo": "redo-default.svg",
    "Ocr": "text_snippet-fill1.svg",
    "Pin": "push_pin-fill1.svg",
    "Record": "videocam-fill1.svg",
    "Copy": "content_copy-default.svg",
    "Save": "save-fill1.svg",
    "Cancel": "close-default.svg",
}


def parse_material(filename):
    """Material SVGs: paths without a fill attribute are solid shapes."""
    import xml.etree.ElementTree as ET
    import re
    from gen_icons import (PathBuilder, SVG_NS, arc_to_cubics)

    root = ET.parse(filename).getroot()
    view = root.get("viewBox", "0 -960 960 960").split()
    vx, vy, vw, vh = (float(v) for v in view)

    def parse_path_data(data, builder):
        tokens = list(gen_icons.tokenize_path(data))
        index, x, y, start_x, start_y = 0, 0.0, 0.0, 0.0, 0.0
        last_cmd, last_c2, last_q1 = None, (0.0, 0.0), (0.0, 0.0)

        def numbers(count):
            nonlocal index
            values = tokens[index:index + count]
            index += count
            return values

        while index < len(tokens):
            token = tokens[index]
            if isinstance(token, str):
                index += 1
                cmd = token
            else:
                cmd = last_cmd
            rel = cmd.islower()
            up = cmd.upper()
            if up == "M":
                px, py = numbers(2)
                if rel:
                    px, py = x + px, y + py
                builder.move(px, py)
                x, y = px, py
                start_x, start_y = px, py
                last_cmd = "l" if cmd.islower() else "L"
            elif up == "L":
                px, py = numbers(2)
                if rel:
                    px, py = x + px, y + py
                builder.line(px, py)
                x, y = px, py
                last_cmd = cmd
            elif up == "H":
                (px,) = numbers(1)
                if rel:
                    px += x
                builder.line(px, y)
                x = px
                last_cmd = cmd
            elif up == "V":
                (py,) = numbers(1)
                if rel:
                    py += y
                builder.line(x, py)
                y = py
                last_cmd = cmd
            elif up == "C":
                a, b, c2d, d, e, f = numbers(6)
                if rel:
                    a, b, c2d, d, e, f = x + a, y + b, x + c2d, y + d, x + e, y + f
                builder.cubic(a, b, c2d, d, e, f)
                last_c2 = (c2d, d)
                x, y = e, f
                last_cmd = cmd
            elif up == "S":
                a, b, c2d, d, e, f = numbers(6)
                if rel:
                    a, b, c2d, d, e, f = x + a, y + b, x + c2d, y + d, x + e, y + f
                r2 = (2 * x - last_c2[0], 2 * y - last_c2[1]) \
                    if last_cmd in "CcSs" else (x, y)
                builder.cubic(r2[0], r2[1], c2d, d, e, f)
                last_c2 = (c2d, d)
                x, y = e, f
                last_cmd = cmd
            elif up == "Q":
                a, b, c2d, d = numbers(4)
                if rel:
                    a, b, c2d, d = x + a, y + b, x + c2d, y + d
                last_q1 = (a, b)
                builder.cubic(x + 2 / 3 * (a - x), y + 2 / 3 * (b - y),
                              c2d + 2 / 3 * (a - c2d), d + 2 / 3 * (b - d),
                              c2d, d)
                x, y = c2d, d
                last_cmd = cmd
            elif up == "T":
                a, b = numbers(2)
                if rel:
                    a, b = x + a, y + b
                q1 = (2 * x - last_q1[0], 2 * y - last_q1[1]) \
                    if last_cmd in "QqTt" else (x, y)
                last_q1 = q1
                builder.cubic(x + 2 / 3 * (q1[0] - x), y + 2 / 3 * (q1[1] - y),
                              a + 2 / 3 * (q1[0] - a), b + 2 / 3 * (q1[1] - b),
                              a, b)
                x, y = a, b
                last_cmd = cmd
            elif up == "A":
                rx, ry, rot, large, sweep, px, py = numbers(7)
                if rel:
                    px, py = x + px, y + py
                for cubic in gen_icons.arc_to_cubics(
                        x, y, rx, ry, rot, large, sweep, px, py):
                    builder.cubic(*cubic)
                x, y = px, py
                last_cmd = cmd
            elif up == "Z":
                builder.close()
                x, y = start_x, start_y
                last_cmd = cmd

    fills = []

    def walk(element):
        tag = element.tag.replace(SVG_NS, "")
        if tag == "path":
            builder = PathBuilder()
            parse_path_data(element.get("d", ""), builder)
            fills.append(builder.prims)
        elif tag in ("circle", "ellipse", "rect"):
            builder = PathBuilder()
            if tag == "rect":
                builder.rounded_rect(
                    float(element.get("x", 0)), float(element.get("y", 0)),
                    float(element.get("width", 0)), float(element.get("height", 0)),
                    float(element.get("rx", 0) or 0),
                    float(element.get("ry", 0) or element.get("rx", 0) or 0) or None)
            elif tag == "circle":
                r = float(element.get("r", 0))
                builder.ellipse(float(element.get("cx", 0)),
                                float(element.get("cy", 0)), r, r)
            else:
                builder.ellipse(float(element.get("cx", 0)),
                                float(element.get("cy", 0)),
                                float(element.get("rx", 0)),
                                float(element.get("ry", 0)))
            fills.append(builder.prims)
        for child in element:
            walk(child)

    for child in root:
        walk(child)

    def normalize(prims):
        sx, sy = 24.0 / vw, 24.0 / vh
        out = []
        for prim in prims:
            if prim[0] == "Z":
                out.append(("Z",))
            elif prim[0] in ("M", "L"):
                out.append((prim[0], (prim[1] - vx) * sx, (prim[2] - vy) * sy))
            else:
                out.append(("C",) + tuple(
                    (v - (vx if i % 2 == 0 else vy)) * (sx if i % 2 == 0 else sy)
                    for i, v in enumerate(prim[1:])))
        return out

    return [normalize(part) for part in fills]


if __name__ == "__main__":
    parts_tables = []
    for slot, filename in FINAL.items():
        parts = parse_material(os.path.join(MATERIAL_DIR, "mat-" + filename))
        parts_tables.append((slot, filename, parts))
        print(slot, "parts:", [len(p) for p in parts])

    seg_defs = []
    part_defs = []
    lookup = []
    for slot, filename, parts in parts_tables:
        part_entries = []
        for index, prims in enumerate(parts):
            seg_name = f"k{slot}Segs{index}"
            seg_defs.append(
                f"const Seg {seg_name}[] = {{\n{gen_icons.emit_prims(prims)}\n}};\n")
            part_entries.append(f"{{{seg_name}, {len(prims)}, true}}")
        part_defs.append(
            f"const GlyphPart k{slot}Parts[] = {{\n    "
            + ",\n    ".join(part_entries) + "\n}};\n")
        lookup.append(
            f"    case ToolbarIcon::{slot}: "
            f"return {{k{slot}Parts, {len(parts)}}};")

    cpp = f'''#include "capture/ToolbarIcons.h"

#include <QPainter>
#include <QPainterPath>

#include <cstddef>

// Glyph geometry adopted from the Material Symbols Rounded set,
// https://fonts.google.com/icons (Apache-2.0, see
// licenses/Material-Symbols-Apache-2.0.txt), fetched 2026-10-02 from the
// fonts.gstatic.com short-term release endpoint. The 960-unit SVG paths
// were converted to 24-grid segment tables; tools use the fill1 weight
// (solid silhouette, the Snipaste-like mixed language), geometric shapes
// use the default outlined weight. Rendering scales the 24 grid to the
// requested logical size x device pixel ratio.

namespace snipnexs {{

namespace {{

struct Seg {{
    enum Kind {{ Move, Line, Cubic, Close }} kind;
    float p[6];
}};

struct GlyphPart {{
    const Seg* segs;
    std::size_t count;
    bool fill;
}};

struct Glyph {{
    const GlyphPart* parts;
    std::size_t count;
}};

{"".join(seg_defs)}
{"".join(part_defs)}
Glyph glyphFor(ToolbarIcon icon)
{{
    switch (icon) {{
{chr(10).join(lookup)}
    }}
    return {{nullptr, 0}};
}}

}} // namespace

QPixmap drawToolbarIconAt(
    ToolbarIcon icon, const QColor& color, int logicalSize, qreal devicePixelRatio)
{{
    const qreal deviceSize = logicalSize * devicePixelRatio;
    QPixmap pixmap(qRound(deviceSize), qRound(deviceSize));
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(devicePixelRatio);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    // A DPR-tagged pixmap paints in LOGICAL coordinates; scaling by
    // deviceSize/24 would double-apply the ratio and clip the glyph.
    const qreal scale = static_cast<qreal>(logicalSize) / 24.0;
    painter.scale(scale, scale);
    const Glyph glyph = glyphFor(icon);
    for (std::size_t part = 0; part < glyph.count; ++part) {{
        const GlyphPart& glyphPart = glyph.parts[part];
        QPainterPath path;
        for (std::size_t i = 0; i < glyphPart.count; ++i) {{
            const Seg& segment = glyphPart.segs[i];
            switch (segment.kind) {{
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
            }}
        }}
        if (glyphPart.fill) {{
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
        }} else {{
            painter.setPen(QPen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.setBrush(Qt::NoBrush);
        }}
        painter.drawPath(path);
    }}
    painter.end();
    return pixmap;
}}

QPixmap drawToolbarIcon(ToolbarIcon icon, const QColor& color)
{{
    return drawToolbarIconAt(icon, color, 24, 2.0);
}}

QIcon makeToolbarIcon(ToolbarIcon icon, bool onDarkBackground)
{{
    QIcon result;
    const QColor normal = onDarkBackground
        ? QColor(240, 244, 249)
        : QColor(53, 65, 76);
    const QColor active = onDarkBackground
        ? QColor(255, 255, 255)
        : QColor(20, 29, 37);
    const QColor disabled = onDarkBackground
        ? QColor(240, 244, 249, 90)
        : QColor(158, 168, 177);
    for (const int logicalSize : {{20, 23}}) {{
        for (const qreal scaleFactor : {{1.0, 1.25, 1.5, 2.0}}) {{
            result.addPixmap(drawToolbarIconAt(
                icon, normal, logicalSize, scaleFactor), QIcon::Normal, QIcon::Off);
            result.addPixmap(drawToolbarIconAt(
                icon, active, logicalSize, scaleFactor), QIcon::Active, QIcon::Off);
            result.addPixmap(drawToolbarIconAt(
                icon, disabled, logicalSize, scaleFactor), QIcon::Disabled, QIcon::Off);
            result.addPixmap(drawToolbarIconAt(
                icon, QColor(255, 255, 255), logicalSize, scaleFactor),
                QIcon::Normal, QIcon::On);
            result.addPixmap(drawToolbarIconAt(
                icon, QColor(255, 255, 255), logicalSize, scaleFactor),
                QIcon::Active, QIcon::On);
        }}
    }}
    return result;
}}

}} // namespace snipnexs
'''
    io.open(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                     "..", "..", "src", "capture", "ToolbarIcons.cpp"), "w",
            encoding="utf-8", newline="\n").write(cpp)
    print("ToolbarIcons.cpp generated (parts-based)")
