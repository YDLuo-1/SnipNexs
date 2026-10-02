"""Generates QPainterPath command tables from icon-library SVGs.

Parses the subset of SVG used by Lucide/Tabler (path/circle/ellipse/rect/
line/polyline on a 24x24 grid), converts SVG endpoint arcs to cubic
beziers, and emits C++ segment tables plus a candidates preview.
"""
import math
import re
import xml.etree.ElementTree as ET

SVG_NS = "{http://www.w3.org/2000/svg}"


def tokenize_path(data):
    for match in re.finditer(
            r"([MmLlHhVvCcSsQqTtAaZz])|(-?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?)",
            data):
        if match.group(1):
            yield match.group(1)
        else:
            yield float(match.group(2))


def arc_to_cubics(x1, y1, rx, ry, phi_deg, large_arc, sweep, x2, y2):
    """SVG endpoint-parameterized arc -> list of cubic segments."""
    if rx == 0.0 or ry == 0.0 or (x1 == x2 and y1 == y2):
        return []
    rx, ry = abs(rx), abs(ry)
    phi = math.radians(phi_deg % 360.0)
    cos_p, sin_p = math.cos(phi), math.sin(phi)

    dx2, dy2 = (x1 - x2) / 2.0, (y1 - y2) / 2.0
    x1p = cos_p * dx2 + sin_p * dy2
    y1p = -sin_p * dx2 + cos_p * dy2

    lam = x1p * x1p / (rx * rx) + y1p * y1p / (ry * ry)
    if lam > 1.0:
        root = math.sqrt(lam)
        rx *= root
        ry *= root

    num = rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p
    den = rx * rx * y1p * y1p + ry * ry * x1p * x1p
    co = math.sqrt(max(0.0, num / den)) if den else 0.0
    if large_arc == sweep:
        co = -co
    cxp, cyp = co * rx * y1p / ry, -co * ry * x1p / rx
    cx = cos_p * cxp - sin_p * cyp + (x1 + x2) / 2.0
    cy = sin_p * cxp + cos_p * cyp + (y1 + y2) / 2.0

    def angle(ux, uy, vx, vy):
        dot, length = ux * vx + uy * vy, math.hypot(ux, uy) * math.hypot(vx, vy)
        value = math.acos(max(-1.0, min(1.0, dot / length))) if length else 0.0
        if ux * vy - uy * vx < 0.0:
            value = -value
        return value

    theta = angle(1.0, 0.0, (x1p - cxp) / rx, (y1p - cyp) / ry)
    delta = angle((x1p - cxp) / rx, (y1p - cyp) / ry,
                  (-x1p - cxp) / rx, (-y1p - cyp) / ry)
    if not sweep and delta > 0.0:
        delta -= 2.0 * math.pi
    elif sweep and delta < 0.0:
        delta += 2.0 * math.pi

    segments = math.ceil(abs(delta) / (math.pi / 2.0)) or 1
    step = delta / segments
    kappa = 4.0 / 3.0 * math.tan(step / 4.0)

    def point(t):
        return (cx + rx * math.cos(t) * cos_p - ry * math.sin(t) * sin_p,
                cy + rx * math.cos(t) * sin_p + ry * math.sin(t) * cos_p)

    def tangent(t):
        return (-rx * math.sin(t) * cos_p - ry * math.cos(t) * sin_p,
                -rx * math.sin(t) * sin_p + ry * math.cos(t) * cos_p)

    cubics = []
    t = theta
    for _ in range(segments):
        p1, p2 = point(t), point(t + step)
        d1, d2 = tangent(t), tangent(t + step)
        cubics.append((p1[0] + kappa * d1[0], p1[1] + kappa * d1[1],
                       p2[0] - kappa * d2[0], p2[1] - kappa * d2[1],
                       p2[0], p2[1]))
        t += step
    return cubics


class PathBuilder:
    """Produces primitive lists: ('M',x,y) ('L',x,y) ('C',x1,y1,x2,y2,x,y) ('Z',)"""

    def __init__(self):
        self.prims = []

    def move(self, x, y):
        self.prims.append(("M", x, y))

    def line(self, x, y):
        self.prims.append(("L", x, y))

    def cubic(self, x1, y1, x2, y2, x, y):
        self.prims.append(("C", x1, y1, x2, y2, x, y))

    def close(self):
        self.prims.append(("Z",))

    def rounded_rect(self, x, y, w, h, rx, ry=None):
        ry = ry if ry is not None else rx
        if rx <= 0 or ry <= 0:
            self.move(x, y); self.line(x + w, y); self.line(x + w, y + h)
            self.line(x, y + h); self.close(); return
        rx, ry = min(rx, w / 2), min(ry, h / 2)
        k = 0.5522847498
        self.move(x + rx, y)
        self.line(x + w - rx, y)
        self.cubic(x + w - rx + k * rx, y, x + w, y + ry - k * ry, x + w, y + ry)
        self.line(x + w, y + h - ry)
        self.cubic(x + w, y + h - ry + k * ry, x + w - rx + k * rx, y + h,
                   x + w - rx, y + h)
        self.line(x + rx, y + h)
        self.cubic(x + rx - k * rx, y + h, x, y + h - ry + k * ry, x, y + h - ry)
        self.line(x, y + ry)
        self.cubic(x, y + ry - k * ry, x + rx - k * rx, y, x + rx, y)
        self.close()

    def ellipse(self, cx, cy, rx, ry):
        k = 0.5522847498
        self.move(cx + rx, cy)
        self.cubic(cx + rx, cy + k * ry, cx + k * rx, cy + ry, cx, cy + ry)
        self.cubic(cx - k * rx, cy + ry, cx - rx, cy + k * ry, cx - rx, cy)
        self.cubic(cx - rx, cy - k * ry, cx - k * rx, cy - ry, cx, cy - ry)
        self.cubic(cx + k * rx, cy - ry, cx + rx, cy - k * ry, cx + rx, cy)
        self.close()


def parse_path_data(data, builder):
    tokens = list(tokenize_path(data))
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
            cmd = last_cmd  # implicit repeat
        rel = cmd.islower()
        c = cmd.upper()
        if c == "M":
            px, py = numbers(2)
            if rel:
                px, py = x + px, y + py
            builder.move(px, py)
            x, y = px, py
            start_x, start_y = px, py
            # Subsequent implicit pairs after m/M are linetos in the same
            # case (relative for m, absolute for M).
            last_cmd = "l" if cmd.islower() else "L"
        elif c == "L":
            px, py = numbers(2)
            if rel:
                px, py = x + px, y + py
            builder.line(px, py)
            x, y = px, py
            last_cmd = cmd
        elif c == "H":
            (px,) = numbers(1)
            if rel:
                px += x
            builder.line(px, y)
            x = px
            last_cmd = cmd
        elif c == "V":
            (py,) = numbers(1)
            if rel:
                py += y
            builder.line(x, py)
            y = py
            last_cmd = cmd
        elif c == "C":
            a, b, c2d, d, e, f = numbers(6)
            if rel:
                a, b, c2d, d, e, f = x + a, y + b, x + c2d, y + d, x + e, y + f
            builder.cubic(a, b, c2d, d, e, f)
            last_c2 = (c2d, d)
            x, y = e, f
            last_cmd = cmd
        elif c == "S":
            a, b, c2d, d, e, f = numbers(6)
            if rel:
                a, b, c2d, d, e, f = x + a, y + b, x + c2d, y + d, x + e, y + f
            r2 = (2 * x - last_c2[0], 2 * y - last_c2[1]) \
                if last_cmd in "CcSs" else (x, y)
            builder.cubic(r2[0], r2[1], c2d, d, e, f)
            last_c2 = (c2d, d)
            x, y = e, f
            last_cmd = cmd
        elif c == "Q":
            a, b, c2d, d = numbers(4)
            if rel:
                a, b, c2d, d = x + a, y + b, x + c2d, y + d
            last_q1 = (a, b)
            builder.cubic(x + 2 / 3 * (a - x), y + 2 / 3 * (b - y),
                          c2d + 2 / 3 * (a - c2d), d + 2 / 3 * (b - d),
                          c2d, d)
            x, y = c2d, d
            last_cmd = cmd
        elif c == "T":
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
        elif c == "A":
            rx, ry, rot, large, sweep, px, py = numbers(7)
            if rel:
                px, py = x + px, y + py
            for cubic in arc_to_cubics(x, y, rx, ry, rot, large, sweep, px, py):
                builder.cubic(*cubic)
            x, y = px, py
            last_cmd = cmd
        elif c == "Z":
            builder.close()
            x, y = start_x, start_y
            last_cmd = cmd
    return builder


def parse_svg(filename):
    """Returns (primitives, filled_primitives) in the SVG's own coordinates."""
    root = ET.parse(filename).getroot()
    view = root.get("viewBox", "0 0 24 24").split()
    vx, vy, vw, vh = (float(v) for v in view)

    stroke = PathBuilder()
    fills = []

    def walk(element):
        tag = element.tag.replace(SVG_NS, "")
        if tag == "path":
            parse_path_data(element.get("d", ""), stroke)
        elif tag == "rect":
            builder = PathBuilder()
            builder.rounded_rect(float(element.get("x", 0)),
                                 float(element.get("y", 0)),
                                 float(element.get("width", 0)),
                                 float(element.get("height", 0)),
                                 float(element.get("rx", 0) or 0),
                                 float(element.get("ry", 0) or element.get("rx", 0) or 0) or None)
            if element.get("fill", "none") != "none":
                fills.extend(builder.prims)
            else:
                stroke.prims.extend(builder.prims)
        elif tag in ("circle", "ellipse"):
            builder = PathBuilder()
            cx = float(element.get("cx", 0))
            cy = float(element.get("cy", 0))
            if tag == "circle":
                r = float(element.get("r", 0))
                builder.ellipse(cx, cy, r, r)
            else:
                builder.ellipse(cx, cy, float(element.get("rx", 0)),
                                float(element.get("ry", 0)))
            if element.get("fill", "none") != "none":
                fills.extend(builder.prims)
            else:
                stroke.prims.extend(builder.prims)
        elif tag == "line":
            builder = PathBuilder()
            builder.move(float(element.get("x1")), float(element.get("y1")))
            builder.line(float(element.get("x2")), float(element.get("y2")))
            stroke.prims.extend(builder.prims)
        elif tag in ("polyline", "polygon"):
            builder = PathBuilder()
            coords = [float(v) for v in re.split(r"[ ,]+",
                                                 element.get("points", "").strip())
                      if v]
            if len(coords) >= 4:
                builder.move(coords[0], coords[1])
                for i in range(2, len(coords), 2):
                    builder.line(coords[i], coords[i + 1])
                if tag == "polygon":
                    builder.close()
            stroke.prims.extend(builder.prims)
        for child in element:
            walk(child)

    for child in root:
        walk(child)

    # Normalize into the 24x24 grid used by both icon sets.
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

    return normalize(stroke.prims), normalize(fills)


def fmt(value):
    text = f"{value:.3g}"
    if re.fullmatch(r"-?\d+", text):
        text += ".0"
    return text + "f"


def emit_prims(prims, indent="    "):
    lines = []
    for prim in prims:
        if prim[0] == "Z":
            lines.append(f"{indent}{{Seg::Close, {{}}}},")
        elif prim[0] in ("M", "L"):
            kind = "Move" if prim[0] == "M" else "Line"
            lines.append(f"{indent}{{Seg::{kind}, {{{fmt(prim[1])}, {fmt(prim[2])}}}}},")
        else:
            coords = ", ".join(fmt(v) for v in prim[1:])
            lines.append(f"{indent}{{Seg::Cubic, {{{coords}}}}},")
    return "\n".join(lines)


# slot -> list of (variant label, svg file)
CANDIDATES = {
    "Pen": [("lucide-pencil", "lucide-pencil.svg"),
            ("lucide-pencil-line", "lucide-pencil-line.svg"),
            ("tabler-pencil", "tabler-pencil.svg")],
    "Text": [("lucide-text-cursor", "lucide-text-cursor.svg"),
             ("tabler-cursor-text", "tabler-cursor-text.svg")],
    "ColorPicker": [("lucide-pipette", "lucide-pipette.svg"),
                    ("lucide-palette", "lucide-palette.svg")],
    "Undo": [("lucide-undo-2", "lucide-undo-2.svg"),
             ("tabler-arrow-back-up", "tabler-arrow-back-up.svg")],
    "Redo": [("lucide-redo-2", "lucide-redo-2.svg"),
             ("tabler-arrow-forward-up", "tabler-arrow-forward-up.svg")],
    "Ocr": [("lucide-scan-text", "lucide-scan-text.svg"),
            ("tabler-scan", "tabler-scan.svg")],
    "Pin": [("lucide-pin", "lucide-pin.svg"),
            ("tabler-pin", "tabler-pin.svg")],
    "Record": [("lucide-video", "lucide-video.svg")],
    "Copy": [("lucide-copy", "lucide-copy.svg")],
    "Save": [("lucide-save", "lucide-save.svg"),
             ("tabler-device-floppy", "tabler-device-floppy.svg")],
    "Rectangle": [("lucide-square", "lucide-square.svg")],
    "Arrow": [("lucide-arrow-up-right", "lucide-arrow-up-right.svg")],
    "Cancel": [("lucide-x", "lucide-x.svg")],
    "Check": [("lucide-check", "lucide-check.svg")],
}

if __name__ == "__main__":
    for slot, variants in CANDIDATES.items():
        for label, filename in variants:
            try:
                prims, fills = parse_svg(filename)
                print(f"{label}: {len(prims)} stroke prims, {len(fills)} fill prims")
            except Exception as error:  # noqa: BLE001
                print(f"{label}: FAILED {error}")
