#include "capture/ToolbarIcons.h"

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

namespace snipnexs {

namespace {

struct Seg {
    enum Kind { Move, Line, Cubic, Close } kind;
    float p[6];
};

struct GlyphPart {
    const Seg* segs;
    std::size_t count;
    bool fill;
};

struct Glyph {
    const GlyphPart* parts;
    std::size_t count;
};

const Seg kPenSegs0[] = {
    {Seg::Move, {3.75f, 21.0f}},
    {Seg::Cubic, {3.53f, 21.0f, 3.35f, 20.9f, 3.21f, 20.8f}},
    {Seg::Cubic, {3.07f, 20.6f, 3.0f, 20.5f, 3.0f, 20.2f}},
    {Seg::Line, {3.0f, 18.4f}},
    {Seg::Cubic, {3.0f, 18.2f, 3.04f, 18.0f, 3.12f, 17.8f}},
    {Seg::Cubic, {3.21f, 17.6f, 3.32f, 17.5f, 3.45f, 17.4f}},
    {Seg::Line, {17.4f, 3.45f}},
    {Seg::Cubic, {17.5f, 3.32f, 17.7f, 3.21f, 17.9f, 3.14f}},
    {Seg::Cubic, {18.0f, 3.06f, 18.2f, 3.03f, 18.4f, 3.03f}},
    {Seg::Cubic, {18.6f, 3.03f, 18.8f, 3.06f, 19.0f, 3.14f}},
    {Seg::Cubic, {19.2f, 3.21f, 19.3f, 3.32f, 19.5f, 3.45f}},
    {Seg::Line, {20.6f, 4.55f}},
    {Seg::Cubic, {20.7f, 4.7f, 20.8f, 4.87f, 20.9f, 5.05f}},
    {Seg::Cubic, {21.0f, 5.23f, 21.0f, 5.42f, 21.0f, 5.6f}},
    {Seg::Cubic, {21.0f, 5.78f, 21.0f, 5.97f, 20.9f, 6.16f}},
    {Seg::Cubic, {20.8f, 6.35f, 20.7f, 6.52f, 20.6f, 6.65f}},
    {Seg::Line, {6.65f, 20.6f}},
    {Seg::Cubic, {6.52f, 20.7f, 6.35f, 20.8f, 6.16f, 20.9f}},
    {Seg::Cubic, {5.97f, 21.0f, 5.78f, 21.0f, 5.58f, 21.0f}},
    {Seg::Line, {3.75f, 21.0f}},
    {Seg::Close, {}},
    {Seg::Move, {18.5f, 6.58f}},
    {Seg::Line, {19.5f, 5.58f}},
    {Seg::Line, {18.4f, 4.55f}},
    {Seg::Line, {17.4f, 5.55f}},
    {Seg::Line, {18.5f, 6.58f}},
    {Seg::Close, {}},
};
const Seg kRectangleSegs0[] = {
    {Seg::Move, {4.5f, 21.0f}},
    {Seg::Cubic, {4.1f, 21.0f, 3.75f, 20.9f, 3.45f, 20.6f}},
    {Seg::Cubic, {3.15f, 20.2f, 3.0f, 19.9f, 3.0f, 19.5f}},
    {Seg::Line, {3.0f, 4.5f}},
    {Seg::Cubic, {3.0f, 4.1f, 3.15f, 3.75f, 3.45f, 3.45f}},
    {Seg::Cubic, {3.75f, 3.15f, 4.1f, 3.0f, 4.5f, 3.0f}},
    {Seg::Line, {19.5f, 3.0f}},
    {Seg::Cubic, {19.9f, 3.0f, 20.2f, 3.15f, 20.6f, 3.45f}},
    {Seg::Cubic, {20.9f, 3.75f, 21.0f, 4.1f, 21.0f, 4.5f}},
    {Seg::Line, {21.0f, 19.5f}},
    {Seg::Cubic, {21.0f, 19.9f, 20.9f, 20.2f, 20.6f, 20.6f}},
    {Seg::Cubic, {20.2f, 20.9f, 19.9f, 21.0f, 19.5f, 21.0f}},
    {Seg::Line, {4.5f, 21.0f}},
    {Seg::Close, {}},
    {Seg::Move, {4.5f, 19.5f}},
    {Seg::Line, {19.5f, 19.5f}},
    {Seg::Line, {19.5f, 4.5f}},
    {Seg::Line, {4.5f, 4.5f}},
    {Seg::Line, {4.5f, 19.5f}},
    {Seg::Close, {}},
    {Seg::Move, {4.5f, 19.5f}},
    {Seg::Line, {4.5f, 4.5f}},
    {Seg::Line, {4.5f, 19.5f}},
    {Seg::Close, {}},
};
const Seg kArrowSegs0[] = {
    {Seg::Move, {16.4f, 7.55f}},
    {Seg::Line, {6.58f, 17.3f}},
    {Seg::Cubic, {6.43f, 17.5f, 6.25f, 17.6f, 6.05f, 17.6f}},
    {Seg::Cubic, {5.85f, 17.6f, 5.68f, 17.5f, 5.53f, 17.3f}},
    {Seg::Cubic, {5.38f, 17.2f, 5.3f, 17.0f, 5.3f, 16.8f}},
    {Seg::Cubic, {5.3f, 16.6f, 5.38f, 16.4f, 5.53f, 16.3f}},
    {Seg::Line, {15.3f, 6.5f}},
    {Seg::Line, {6.6f, 6.5f}},
    {Seg::Cubic, {6.39f, 6.5f, 6.21f, 6.43f, 6.07f, 6.28f}},
    {Seg::Cubic, {5.92f, 6.14f, 5.85f, 5.96f, 5.85f, 5.75f}},
    {Seg::Cubic, {5.85f, 5.53f, 5.92f, 5.35f, 6.07f, 5.21f}},
    {Seg::Cubic, {6.21f, 5.07f, 6.39f, 5.0f, 6.6f, 5.0f}},
    {Seg::Line, {17.1f, 5.0f}},
    {Seg::Cubic, {17.3f, 5.0f, 17.5f, 5.07f, 17.6f, 5.22f}},
    {Seg::Cubic, {17.8f, 5.36f, 17.9f, 5.54f, 17.9f, 5.75f}},
    {Seg::Line, {17.9f, 16.2f}},
    {Seg::Cubic, {17.9f, 16.5f, 17.8f, 16.6f, 17.6f, 16.8f}},
    {Seg::Cubic, {17.5f, 16.9f, 17.3f, 17.0f, 17.1f, 17.0f}},
    {Seg::Cubic, {16.9f, 17.0f, 16.7f, 16.9f, 16.6f, 16.8f}},
    {Seg::Cubic, {16.4f, 16.6f, 16.4f, 16.5f, 16.4f, 16.2f}},
    {Seg::Line, {16.4f, 7.55f}},
    {Seg::Close, {}},
};
const Seg kTextSegs0[] = {
    {Seg::Move, {7.61f, 19.6f}},
    {Seg::Cubic, {7.37f, 19.4f, 7.25f, 19.1f, 7.25f, 18.8f}},
    {Seg::Line, {7.25f, 6.5f}},
    {Seg::Line, {3.25f, 6.5f}},
    {Seg::Cubic, {2.9f, 6.5f, 2.61f, 6.38f, 2.36f, 6.13f}},
    {Seg::Cubic, {2.12f, 5.89f, 2.0f, 5.59f, 2.0f, 5.25f}},
    {Seg::Cubic, {2.0f, 4.9f, 2.12f, 4.6f, 2.36f, 4.36f}},
    {Seg::Cubic, {2.61f, 4.12f, 2.9f, 4.0f, 3.25f, 4.0f}},
    {Seg::Line, {13.8f, 4.0f}},
    {Seg::Cubic, {14.1f, 4.0f, 14.4f, 4.12f, 14.6f, 4.37f}},
    {Seg::Cubic, {14.9f, 4.61f, 15.0f, 4.91f, 15.0f, 5.25f}},
    {Seg::Cubic, {15.0f, 5.6f, 14.9f, 5.9f, 14.6f, 6.14f}},
    {Seg::Cubic, {14.4f, 6.38f, 14.1f, 6.5f, 13.8f, 6.5f}},
    {Seg::Line, {9.75f, 6.5f}},
    {Seg::Line, {9.75f, 18.8f}},
    {Seg::Cubic, {9.75f, 19.1f, 9.63f, 19.4f, 9.38f, 19.6f}},
    {Seg::Cubic, {9.14f, 19.9f, 8.85f, 20.0f, 8.5f, 20.0f}},
    {Seg::Cubic, {8.15f, 20.0f, 7.85f, 19.9f, 7.61f, 19.6f}},
    {Seg::Close, {}},
    {Seg::Move, {16.6f, 19.6f}},
    {Seg::Cubic, {16.4f, 19.4f, 16.2f, 19.1f, 16.2f, 18.8f}},
    {Seg::Line, {16.2f, 11.5f}},
    {Seg::Line, {14.2f, 11.5f}},
    {Seg::Cubic, {13.9f, 11.5f, 13.6f, 11.4f, 13.4f, 11.1f}},
    {Seg::Cubic, {13.1f, 10.9f, 13.0f, 10.6f, 13.0f, 10.2f}},
    {Seg::Cubic, {13.0f, 9.9f, 13.1f, 9.6f, 13.4f, 9.36f}},
    {Seg::Cubic, {13.6f, 9.12f, 13.9f, 9.0f, 14.2f, 9.0f}},
    {Seg::Line, {20.8f, 9.0f}},
    {Seg::Cubic, {21.1f, 9.0f, 21.4f, 9.12f, 21.6f, 9.37f}},
    {Seg::Cubic, {21.9f, 9.61f, 22.0f, 9.91f, 22.0f, 10.3f}},
    {Seg::Cubic, {22.0f, 10.6f, 21.9f, 10.9f, 21.6f, 11.1f}},
    {Seg::Cubic, {21.4f, 11.4f, 21.1f, 11.5f, 20.8f, 11.5f}},
    {Seg::Line, {18.8f, 11.5f}},
    {Seg::Line, {18.8f, 18.8f}},
    {Seg::Cubic, {18.8f, 19.1f, 18.6f, 19.4f, 18.4f, 19.6f}},
    {Seg::Cubic, {18.1f, 19.9f, 17.8f, 20.0f, 17.5f, 20.0f}},
    {Seg::Cubic, {17.1f, 20.0f, 16.9f, 19.9f, 16.6f, 19.6f}},
    {Seg::Close, {}},
};
const Seg kColorPickerSegs0[] = {
    {Seg::Move, {3.0f, 20.2f}},
    {Seg::Line, {3.0f, 17.4f}},
    {Seg::Cubic, {3.0f, 17.2f, 3.04f, 17.0f, 3.12f, 16.8f}},
    {Seg::Cubic, {3.21f, 16.6f, 3.32f, 16.5f, 3.45f, 16.4f}},
    {Seg::Line, {12.4f, 7.38f}},
    {Seg::Line, {10.6f, 5.58f}},
    {Seg::Line, {11.7f, 4.55f}},
    {Seg::Line, {14.0f, 6.85f}},
    {Seg::Line, {17.5f, 3.3f}},
    {Seg::Cubic, {17.6f, 3.22f, 17.7f, 3.15f, 17.8f, 3.1f}},
    {Seg::Cubic, {17.9f, 3.05f, 18.0f, 3.03f, 18.1f, 3.03f}},
    {Seg::Cubic, {18.2f, 3.03f, 18.3f, 3.05f, 18.4f, 3.1f}},
    {Seg::Cubic, {18.5f, 3.15f, 18.6f, 3.22f, 18.7f, 3.3f}},
    {Seg::Line, {20.7f, 5.33f}},
    {Seg::Cubic, {20.8f, 5.43f, 20.9f, 5.53f, 20.9f, 5.62f}},
    {Seg::Cubic, {21.0f, 5.73f, 21.0f, 5.83f, 21.0f, 5.93f}},
    {Seg::Cubic, {21.0f, 6.03f, 21.0f, 6.12f, 20.9f, 6.23f}},
    {Seg::Cubic, {20.9f, 6.33f, 20.8f, 6.42f, 20.7f, 6.5f}},
    {Seg::Line, {17.2f, 10.1f}},
    {Seg::Line, {19.5f, 12.4f}},
    {Seg::Line, {18.4f, 13.4f}},
    {Seg::Line, {16.6f, 11.6f}},
    {Seg::Line, {7.65f, 20.6f}},
    {Seg::Cubic, {7.52f, 20.7f, 7.35f, 20.8f, 7.16f, 20.9f}},
    {Seg::Cubic, {6.97f, 21.0f, 6.78f, 21.0f, 6.58f, 21.0f}},
    {Seg::Line, {3.75f, 21.0f}},
    {Seg::Cubic, {3.53f, 21.0f, 3.35f, 20.9f, 3.21f, 20.8f}},
    {Seg::Cubic, {3.07f, 20.6f, 3.0f, 20.5f, 3.0f, 20.2f}},
    {Seg::Close, {}},
    {Seg::Move, {4.5f, 19.5f}},
    {Seg::Line, {6.68f, 19.5f}},
    {Seg::Line, {15.6f, 10.6f}},
    {Seg::Line, {13.4f, 8.45f}},
    {Seg::Line, {4.5f, 17.3f}},
    {Seg::Line, {4.5f, 19.5f}},
    {Seg::Close, {}},
};
const Seg kUndoSegs0[] = {
    {Seg::Move, {7.23f, 19.0f}},
    {Seg::Cubic, {7.01f, 19.0f, 6.83f, 18.9f, 6.69f, 18.8f}},
    {Seg::Cubic, {6.55f, 18.6f, 6.48f, 18.5f, 6.48f, 18.2f}},
    {Seg::Cubic, {6.48f, 18.0f, 6.55f, 17.9f, 6.69f, 17.7f}},
    {Seg::Cubic, {6.83f, 17.6f, 7.01f, 17.5f, 7.23f, 17.5f}},
    {Seg::Line, {14.2f, 17.5f}},
    {Seg::Cubic, {15.4f, 17.5f, 16.4f, 17.1f, 17.2f, 16.3f}},
    {Seg::Cubic, {18.1f, 15.6f, 18.5f, 14.6f, 18.5f, 13.5f}},
    {Seg::Cubic, {18.5f, 12.3f, 18.1f, 11.3f, 17.2f, 10.6f}},
    {Seg::Cubic, {16.4f, 9.79f, 15.4f, 9.4f, 14.2f, 9.4f}},
    {Seg::Line, {6.85f, 9.4f}},
    {Seg::Line, {9.18f, 11.7f}},
    {Seg::Cubic, {9.33f, 11.9f, 9.4f, 12.1f, 9.4f, 12.2f}},
    {Seg::Cubic, {9.4f, 12.5f, 9.33f, 12.6f, 9.18f, 12.8f}},
    {Seg::Cubic, {9.03f, 12.9f, 8.85f, 13.0f, 8.65f, 13.0f}},
    {Seg::Cubic, {8.45f, 13.0f, 8.28f, 12.9f, 8.12f, 12.8f}},
    {Seg::Line, {4.53f, 9.18f}},
    {Seg::Cubic, {4.44f, 9.09f, 4.38f, 9.01f, 4.35f, 8.93f}},
    {Seg::Cubic, {4.32f, 8.84f, 4.3f, 8.75f, 4.3f, 8.65f}},
    {Seg::Cubic, {4.3f, 8.55f, 4.32f, 8.46f, 4.35f, 8.38f}},
    {Seg::Cubic, {4.38f, 8.29f, 4.44f, 8.21f, 4.53f, 8.12f}},
    {Seg::Line, {8.12f, 4.53f}},
    {Seg::Cubic, {8.28f, 4.38f, 8.45f, 4.3f, 8.65f, 4.3f}},
    {Seg::Cubic, {8.85f, 4.3f, 9.03f, 4.38f, 9.18f, 4.53f}},
    {Seg::Cubic, {9.33f, 4.67f, 9.4f, 4.85f, 9.4f, 5.05f}},
    {Seg::Cubic, {9.4f, 5.25f, 9.33f, 5.43f, 9.18f, 5.58f}},
    {Seg::Line, {6.85f, 7.9f}},
    {Seg::Line, {14.2f, 7.9f}},
    {Seg::Cubic, {15.8f, 7.9f, 17.1f, 8.43f, 18.3f, 9.5f}},
    {Seg::Cubic, {19.4f, 10.6f, 20.0f, 11.9f, 20.0f, 13.5f}},
    {Seg::Cubic, {20.0f, 15.0f, 19.4f, 16.3f, 18.3f, 17.4f}},
    {Seg::Cubic, {17.1f, 18.5f, 15.8f, 19.0f, 14.2f, 19.0f}},
    {Seg::Line, {7.23f, 19.0f}},
    {Seg::Close, {}},
};
const Seg kRedoSegs0[] = {
    {Seg::Move, {17.2f, 9.4f}},
    {Seg::Line, {9.78f, 9.4f}},
    {Seg::Cubic, {8.61f, 9.4f, 7.6f, 9.79f, 6.76f, 10.6f}},
    {Seg::Cubic, {5.92f, 11.3f, 5.5f, 12.3f, 5.5f, 13.5f}},
    {Seg::Cubic, {5.5f, 14.6f, 5.92f, 15.6f, 6.76f, 16.3f}},
    {Seg::Cubic, {7.6f, 17.1f, 8.61f, 17.5f, 9.78f, 17.5f}},
    {Seg::Line, {16.8f, 17.5f}},
    {Seg::Cubic, {17.0f, 17.5f, 17.2f, 17.6f, 17.3f, 17.7f}},
    {Seg::Cubic, {17.5f, 17.9f, 17.5f, 18.0f, 17.5f, 18.2f}},
    {Seg::Cubic, {17.5f, 18.5f, 17.5f, 18.6f, 17.3f, 18.8f}},
    {Seg::Cubic, {17.2f, 18.9f, 17.0f, 19.0f, 16.8f, 19.0f}},
    {Seg::Line, {9.8f, 19.0f}},
    {Seg::Cubic, {8.22f, 19.0f, 6.85f, 18.5f, 5.71f, 17.4f}},
    {Seg::Cubic, {4.57f, 16.3f, 4.0f, 15.0f, 4.0f, 13.5f}},
    {Seg::Cubic, {4.0f, 11.9f, 4.57f, 10.6f, 5.71f, 9.5f}},
    {Seg::Cubic, {6.85f, 8.43f, 8.22f, 7.9f, 9.8f, 7.9f}},
    {Seg::Line, {17.2f, 7.9f}},
    {Seg::Line, {14.8f, 5.58f}},
    {Seg::Cubic, {14.7f, 5.43f, 14.6f, 5.25f, 14.6f, 5.05f}},
    {Seg::Cubic, {14.6f, 4.85f, 14.7f, 4.67f, 14.8f, 4.53f}},
    {Seg::Cubic, {15.0f, 4.38f, 15.2f, 4.3f, 15.4f, 4.3f}},
    {Seg::Cubic, {15.6f, 4.3f, 15.7f, 4.38f, 15.9f, 4.53f}},
    {Seg::Line, {19.5f, 8.12f}},
    {Seg::Cubic, {19.6f, 8.21f, 19.6f, 8.29f, 19.7f, 8.38f}},
    {Seg::Cubic, {19.7f, 8.46f, 19.7f, 8.55f, 19.7f, 8.65f}},
    {Seg::Cubic, {19.7f, 8.75f, 19.7f, 8.84f, 19.7f, 8.93f}},
    {Seg::Cubic, {19.6f, 9.01f, 19.6f, 9.09f, 19.5f, 9.18f}},
    {Seg::Line, {15.9f, 12.8f}},
    {Seg::Cubic, {15.7f, 12.9f, 15.6f, 13.0f, 15.4f, 13.0f}},
    {Seg::Cubic, {15.2f, 13.0f, 15.0f, 12.9f, 14.8f, 12.8f}},
    {Seg::Cubic, {14.7f, 12.6f, 14.6f, 12.5f, 14.6f, 12.2f}},
    {Seg::Cubic, {14.6f, 12.1f, 14.7f, 11.9f, 14.8f, 11.7f}},
    {Seg::Line, {17.2f, 9.4f}},
    {Seg::Close, {}},
};
const Seg kOcrSegs0[] = {
    {Seg::Move, {2.0f, 4.62f}},
    {Seg::Line, {2.0f, 2.5f}},
    {Seg::Cubic, {2.0f, 2.09f, 2.15f, 1.73f, 2.44f, 1.44f}},
    {Seg::Cubic, {2.73f, 1.15f, 3.09f, 1.0f, 3.5f, 1.0f}},
    {Seg::Line, {5.62f, 1.0f}},
    {Seg::Cubic, {5.84f, 1.0f, 6.02f, 1.07f, 6.16f, 1.22f}},
    {Seg::Cubic, {6.3f, 1.36f, 6.38f, 1.54f, 6.38f, 1.75f}},
    {Seg::Cubic, {6.38f, 1.97f, 6.3f, 2.15f, 6.16f, 2.29f}},
    {Seg::Cubic, {6.02f, 2.43f, 5.84f, 2.5f, 5.62f, 2.5f}},
    {Seg::Line, {3.5f, 2.5f}},
    {Seg::Line, {3.5f, 4.62f}},
    {Seg::Cubic, {3.5f, 4.84f, 3.43f, 5.02f, 3.28f, 5.16f}},
    {Seg::Cubic, {3.14f, 5.3f, 2.96f, 5.38f, 2.75f, 5.38f}},
    {Seg::Cubic, {2.53f, 5.38f, 2.35f, 5.3f, 2.21f, 5.16f}},
    {Seg::Cubic, {2.07f, 5.02f, 2.0f, 4.84f, 2.0f, 4.62f}},
    {Seg::Close, {}},
    {Seg::Move, {20.5f, 4.62f}},
    {Seg::Line, {20.5f, 2.5f}},
    {Seg::Line, {18.4f, 2.5f}},
    {Seg::Cubic, {18.2f, 2.5f, 18.0f, 2.43f, 17.8f, 2.28f}},
    {Seg::Cubic, {17.7f, 2.14f, 17.6f, 1.96f, 17.6f, 1.75f}},
    {Seg::Cubic, {17.6f, 1.53f, 17.7f, 1.35f, 17.8f, 1.21f}},
    {Seg::Cubic, {18.0f, 1.07f, 18.2f, 1.0f, 18.4f, 1.0f}},
    {Seg::Line, {20.5f, 1.0f}},
    {Seg::Cubic, {20.9f, 1.0f, 21.3f, 1.15f, 21.6f, 1.44f}},
    {Seg::Cubic, {21.9f, 1.73f, 22.0f, 2.09f, 22.0f, 2.5f}},
    {Seg::Line, {22.0f, 4.62f}},
    {Seg::Cubic, {22.0f, 4.84f, 21.9f, 5.02f, 21.8f, 5.16f}},
    {Seg::Cubic, {21.6f, 5.3f, 21.5f, 5.38f, 21.2f, 5.38f}},
    {Seg::Cubic, {21.0f, 5.38f, 20.9f, 5.3f, 20.7f, 5.16f}},
    {Seg::Cubic, {20.6f, 5.02f, 20.5f, 4.84f, 20.5f, 4.62f}},
    {Seg::Close, {}},
    {Seg::Move, {2.0f, 21.5f}},
    {Seg::Line, {2.0f, 19.4f}},
    {Seg::Cubic, {2.0f, 19.2f, 2.07f, 19.0f, 2.22f, 18.8f}},
    {Seg::Cubic, {2.36f, 18.7f, 2.54f, 18.6f, 2.75f, 18.6f}},
    {Seg::Cubic, {2.97f, 18.6f, 3.15f, 18.7f, 3.29f, 18.8f}},
    {Seg::Cubic, {3.43f, 19.0f, 3.5f, 19.2f, 3.5f, 19.4f}},
    {Seg::Line, {3.5f, 21.5f}},
    {Seg::Line, {5.62f, 21.5f}},
    {Seg::Cubic, {5.84f, 21.5f, 6.02f, 21.6f, 6.16f, 21.7f}},
    {Seg::Cubic, {6.3f, 21.9f, 6.38f, 22.0f, 6.38f, 22.3f}},
    {Seg::Cubic, {6.38f, 22.5f, 6.3f, 22.6f, 6.16f, 22.8f}},
    {Seg::Cubic, {6.02f, 22.9f, 5.84f, 23.0f, 5.62f, 23.0f}},
    {Seg::Line, {3.5f, 23.0f}},
    {Seg::Cubic, {3.09f, 23.0f, 2.73f, 22.9f, 2.44f, 22.6f}},
    {Seg::Cubic, {2.15f, 22.3f, 2.0f, 21.9f, 2.0f, 21.5f}},
    {Seg::Close, {}},
    {Seg::Move, {20.5f, 23.0f}},
    {Seg::Line, {18.4f, 23.0f}},
    {Seg::Cubic, {18.2f, 23.0f, 18.0f, 22.9f, 17.8f, 22.8f}},
    {Seg::Cubic, {17.7f, 22.6f, 17.6f, 22.5f, 17.6f, 22.2f}},
    {Seg::Cubic, {17.6f, 22.0f, 17.7f, 21.9f, 17.8f, 21.7f}},
    {Seg::Cubic, {18.0f, 21.6f, 18.2f, 21.5f, 18.4f, 21.5f}},
    {Seg::Line, {20.5f, 21.5f}},
    {Seg::Line, {20.5f, 19.4f}},
    {Seg::Cubic, {20.5f, 19.2f, 20.6f, 19.0f, 20.7f, 18.8f}},
    {Seg::Cubic, {20.9f, 18.7f, 21.0f, 18.6f, 21.3f, 18.6f}},
    {Seg::Cubic, {21.5f, 18.6f, 21.6f, 18.7f, 21.8f, 18.8f}},
    {Seg::Cubic, {21.9f, 19.0f, 22.0f, 19.2f, 22.0f, 19.4f}},
    {Seg::Line, {22.0f, 21.5f}},
    {Seg::Cubic, {22.0f, 21.9f, 21.9f, 22.3f, 21.6f, 22.6f}},
    {Seg::Cubic, {21.3f, 22.9f, 20.9f, 23.0f, 20.5f, 23.0f}},
    {Seg::Close, {}},
    {Seg::Move, {6.5f, 20.0f}},
    {Seg::Cubic, {6.1f, 20.0f, 5.75f, 19.9f, 5.45f, 19.6f}},
    {Seg::Cubic, {5.15f, 19.2f, 5.0f, 18.9f, 5.0f, 18.5f}},
    {Seg::Line, {5.0f, 5.5f}},
    {Seg::Cubic, {5.0f, 5.1f, 5.15f, 4.75f, 5.45f, 4.45f}},
    {Seg::Cubic, {5.75f, 4.15f, 6.1f, 4.0f, 6.5f, 4.0f}},
    {Seg::Line, {17.5f, 4.0f}},
    {Seg::Cubic, {17.9f, 4.0f, 18.2f, 4.15f, 18.6f, 4.45f}},
    {Seg::Cubic, {18.9f, 4.75f, 19.0f, 5.1f, 19.0f, 5.5f}},
    {Seg::Line, {19.0f, 18.5f}},
    {Seg::Cubic, {19.0f, 18.9f, 18.9f, 19.2f, 18.6f, 19.6f}},
    {Seg::Cubic, {18.2f, 19.9f, 17.9f, 20.0f, 17.5f, 20.0f}},
    {Seg::Line, {6.5f, 20.0f}},
    {Seg::Close, {}},
    {Seg::Move, {10.0f, 9.75f}},
    {Seg::Line, {14.0f, 9.75f}},
    {Seg::Cubic, {14.2f, 9.75f, 14.4f, 9.68f, 14.5f, 9.53f}},
    {Seg::Cubic, {14.7f, 9.39f, 14.8f, 9.21f, 14.8f, 9.0f}},
    {Seg::Cubic, {14.8f, 8.78f, 14.7f, 8.6f, 14.5f, 8.46f}},
    {Seg::Cubic, {14.4f, 8.32f, 14.2f, 8.25f, 14.0f, 8.25f}},
    {Seg::Line, {10.0f, 8.25f}},
    {Seg::Cubic, {9.79f, 8.25f, 9.61f, 8.32f, 9.47f, 8.47f}},
    {Seg::Cubic, {9.32f, 8.61f, 9.25f, 8.79f, 9.25f, 9.0f}},
    {Seg::Cubic, {9.25f, 9.22f, 9.32f, 9.4f, 9.47f, 9.54f}},
    {Seg::Cubic, {9.61f, 9.68f, 9.79f, 9.75f, 10.0f, 9.75f}},
    {Seg::Close, {}},
    {Seg::Move, {10.0f, 12.8f}},
    {Seg::Line, {14.0f, 12.8f}},
    {Seg::Cubic, {14.2f, 12.8f, 14.4f, 12.7f, 14.5f, 12.5f}},
    {Seg::Cubic, {14.7f, 12.4f, 14.8f, 12.2f, 14.8f, 12.0f}},
    {Seg::Cubic, {14.8f, 11.8f, 14.7f, 11.6f, 14.5f, 11.5f}},
    {Seg::Cubic, {14.4f, 11.3f, 14.2f, 11.2f, 14.0f, 11.2f}},
    {Seg::Line, {10.0f, 11.2f}},
    {Seg::Cubic, {9.79f, 11.2f, 9.61f, 11.3f, 9.47f, 11.5f}},
    {Seg::Cubic, {9.32f, 11.6f, 9.25f, 11.8f, 9.25f, 12.0f}},
    {Seg::Cubic, {9.25f, 12.2f, 9.32f, 12.4f, 9.47f, 12.5f}},
    {Seg::Cubic, {9.61f, 12.7f, 9.79f, 12.8f, 10.0f, 12.8f}},
    {Seg::Close, {}},
    {Seg::Move, {10.0f, 15.8f}},
    {Seg::Line, {14.0f, 15.8f}},
    {Seg::Cubic, {14.2f, 15.8f, 14.4f, 15.7f, 14.5f, 15.5f}},
    {Seg::Cubic, {14.7f, 15.4f, 14.8f, 15.2f, 14.8f, 15.0f}},
    {Seg::Cubic, {14.8f, 14.8f, 14.7f, 14.6f, 14.5f, 14.5f}},
    {Seg::Cubic, {14.4f, 14.3f, 14.2f, 14.2f, 14.0f, 14.2f}},
    {Seg::Line, {10.0f, 14.2f}},
    {Seg::Cubic, {9.79f, 14.2f, 9.61f, 14.3f, 9.47f, 14.5f}},
    {Seg::Cubic, {9.32f, 14.6f, 9.25f, 14.8f, 9.25f, 15.0f}},
    {Seg::Cubic, {9.25f, 15.2f, 9.32f, 15.4f, 9.47f, 15.5f}},
    {Seg::Cubic, {9.61f, 15.7f, 9.79f, 15.8f, 10.0f, 15.8f}},
    {Seg::Close, {}},
};
const Seg kPinSegs0[] = {
    {Seg::Move, {15.9f, 4.5f}},
    {Seg::Line, {15.9f, 12.8f}},
    {Seg::Line, {17.8f, 14.5f}},
    {Seg::Cubic, {17.9f, 14.6f, 17.9f, 14.7f, 17.9f, 14.8f}},
    {Seg::Cubic, {18.0f, 14.9f, 18.0f, 15.0f, 18.0f, 15.1f}},
    {Seg::Line, {18.0f, 15.5f}},
    {Seg::Cubic, {18.0f, 15.7f, 17.9f, 15.9f, 17.8f, 16.0f}},
    {Seg::Cubic, {17.6f, 16.2f, 17.5f, 16.2f, 17.2f, 16.2f}},
    {Seg::Line, {12.8f, 16.2f}},
    {Seg::Line, {12.8f, 22.1f}},
    {Seg::Cubic, {12.8f, 22.3f, 12.7f, 22.5f, 12.5f, 22.7f}},
    {Seg::Cubic, {12.4f, 22.8f, 12.2f, 22.9f, 12.0f, 22.9f}},
    {Seg::Cubic, {11.8f, 22.9f, 11.6f, 22.8f, 11.5f, 22.7f}},
    {Seg::Cubic, {11.3f, 22.5f, 11.2f, 22.3f, 11.2f, 22.1f}},
    {Seg::Line, {11.2f, 16.2f}},
    {Seg::Line, {6.75f, 16.2f}},
    {Seg::Cubic, {6.54f, 16.2f, 6.36f, 16.2f, 6.22f, 16.0f}},
    {Seg::Cubic, {6.07f, 15.9f, 6.0f, 15.7f, 6.0f, 15.5f}},
    {Seg::Line, {6.0f, 15.0f}},
    {Seg::Cubic, {6.0f, 14.9f, 6.02f, 14.9f, 6.05f, 14.8f}},
    {Seg::Cubic, {6.08f, 14.7f, 6.14f, 14.6f, 6.23f, 14.5f}},
    {Seg::Line, {8.0f, 12.8f}},
    {Seg::Line, {8.0f, 4.5f}},
    {Seg::Line, {7.5f, 4.5f}},
    {Seg::Cubic, {7.29f, 4.5f, 7.11f, 4.43f, 6.97f, 4.28f}},
    {Seg::Cubic, {6.82f, 4.14f, 6.75f, 3.96f, 6.75f, 3.75f}},
    {Seg::Cubic, {6.75f, 3.53f, 6.82f, 3.35f, 6.97f, 3.21f}},
    {Seg::Cubic, {7.11f, 3.07f, 7.29f, 3.0f, 7.5f, 3.0f}},
    {Seg::Line, {16.4f, 3.0f}},
    {Seg::Cubic, {16.6f, 3.0f, 16.7f, 3.07f, 16.9f, 3.22f}},
    {Seg::Cubic, {17.0f, 3.36f, 17.1f, 3.54f, 17.1f, 3.75f}},
    {Seg::Cubic, {17.1f, 3.97f, 17.0f, 4.15f, 16.9f, 4.29f}},
    {Seg::Cubic, {16.7f, 4.43f, 16.6f, 4.5f, 16.4f, 4.5f}},
    {Seg::Line, {15.9f, 4.5f}},
    {Seg::Close, {}},
};
const Seg kRecordSegs0[] = {
    {Seg::Move, {3.5f, 20.0f}},
    {Seg::Cubic, {3.1f, 20.0f, 2.75f, 19.9f, 2.45f, 19.6f}},
    {Seg::Cubic, {2.15f, 19.2f, 2.0f, 18.9f, 2.0f, 18.5f}},
    {Seg::Line, {2.0f, 5.5f}},
    {Seg::Cubic, {2.0f, 5.1f, 2.15f, 4.75f, 2.45f, 4.45f}},
    {Seg::Cubic, {2.75f, 4.15f, 3.1f, 4.0f, 3.5f, 4.0f}},
    {Seg::Line, {16.5f, 4.0f}},
    {Seg::Cubic, {16.9f, 4.0f, 17.2f, 4.15f, 17.6f, 4.45f}},
    {Seg::Cubic, {17.9f, 4.75f, 18.0f, 5.1f, 18.0f, 5.5f}},
    {Seg::Line, {18.0f, 10.9f}},
    {Seg::Line, {21.4f, 7.53f}},
    {Seg::Cubic, {21.5f, 7.41f, 21.6f, 7.38f, 21.8f, 7.44f}},
    {Seg::Cubic, {21.9f, 7.5f, 22.0f, 7.61f, 22.0f, 7.78f}},
    {Seg::Line, {22.0f, 16.2f}},
    {Seg::Cubic, {22.0f, 16.4f, 21.9f, 16.5f, 21.8f, 16.6f}},
    {Seg::Cubic, {21.6f, 16.6f, 21.5f, 16.6f, 21.4f, 16.5f}},
    {Seg::Line, {18.0f, 13.1f}},
    {Seg::Line, {18.0f, 18.5f}},
    {Seg::Cubic, {18.0f, 18.9f, 17.9f, 19.2f, 17.6f, 19.6f}},
    {Seg::Cubic, {17.2f, 19.9f, 16.9f, 20.0f, 16.5f, 20.0f}},
    {Seg::Line, {3.5f, 20.0f}},
    {Seg::Close, {}},
};
const Seg kCopySegs0[] = {
    {Seg::Move, {7.5f, 19.0f}},
    {Seg::Cubic, {7.1f, 19.0f, 6.75f, 18.9f, 6.45f, 18.6f}},
    {Seg::Cubic, {6.15f, 18.2f, 6.0f, 17.9f, 6.0f, 17.5f}},
    {Seg::Line, {6.0f, 3.5f}},
    {Seg::Cubic, {6.0f, 3.1f, 6.15f, 2.75f, 6.45f, 2.45f}},
    {Seg::Cubic, {6.75f, 2.15f, 7.1f, 2.0f, 7.5f, 2.0f}},
    {Seg::Line, {18.5f, 2.0f}},
    {Seg::Cubic, {18.9f, 2.0f, 19.2f, 2.15f, 19.6f, 2.45f}},
    {Seg::Cubic, {19.9f, 2.75f, 20.0f, 3.1f, 20.0f, 3.5f}},
    {Seg::Line, {20.0f, 17.5f}},
    {Seg::Cubic, {20.0f, 17.9f, 19.9f, 18.2f, 19.6f, 18.6f}},
    {Seg::Cubic, {19.2f, 18.9f, 18.9f, 19.0f, 18.5f, 19.0f}},
    {Seg::Line, {7.5f, 19.0f}},
    {Seg::Close, {}},
    {Seg::Move, {7.5f, 17.5f}},
    {Seg::Line, {18.5f, 17.5f}},
    {Seg::Line, {18.5f, 3.5f}},
    {Seg::Line, {7.5f, 3.5f}},
    {Seg::Line, {7.5f, 17.5f}},
    {Seg::Close, {}},
    {Seg::Move, {4.5f, 22.0f}},
    {Seg::Cubic, {4.1f, 22.0f, 3.75f, 21.9f, 3.45f, 21.6f}},
    {Seg::Cubic, {3.15f, 21.2f, 3.0f, 20.9f, 3.0f, 20.5f}},
    {Seg::Line, {3.0f, 5.75f}},
    {Seg::Cubic, {3.0f, 5.54f, 3.07f, 5.36f, 3.22f, 5.22f}},
    {Seg::Cubic, {3.36f, 5.07f, 3.54f, 5.0f, 3.75f, 5.0f}},
    {Seg::Cubic, {3.97f, 5.0f, 4.15f, 5.07f, 4.29f, 5.22f}},
    {Seg::Cubic, {4.43f, 5.36f, 4.5f, 5.54f, 4.5f, 5.75f}},
    {Seg::Line, {4.5f, 20.5f}},
    {Seg::Line, {16.2f, 20.5f}},
    {Seg::Cubic, {16.5f, 20.5f, 16.6f, 20.6f, 16.8f, 20.7f}},
    {Seg::Cubic, {16.9f, 20.9f, 17.0f, 21.0f, 17.0f, 21.3f}},
    {Seg::Cubic, {17.0f, 21.5f, 16.9f, 21.6f, 16.8f, 21.8f}},
    {Seg::Cubic, {16.6f, 21.9f, 16.5f, 22.0f, 16.2f, 22.0f}},
    {Seg::Line, {4.5f, 22.0f}},
    {Seg::Close, {}},
    {Seg::Move, {7.5f, 17.5f}},
    {Seg::Line, {7.5f, 3.5f}},
    {Seg::Line, {7.5f, 17.5f}},
    {Seg::Close, {}},
};
const Seg kSaveSegs0[] = {
    {Seg::Move, {4.5f, 21.0f}},
    {Seg::Cubic, {4.1f, 21.0f, 3.75f, 20.9f, 3.45f, 20.6f}},
    {Seg::Cubic, {3.15f, 20.2f, 3.0f, 19.9f, 3.0f, 19.5f}},
    {Seg::Line, {3.0f, 4.5f}},
    {Seg::Cubic, {3.0f, 4.1f, 3.15f, 3.75f, 3.45f, 3.45f}},
    {Seg::Cubic, {3.75f, 3.15f, 4.1f, 3.0f, 4.5f, 3.0f}},
    {Seg::Line, {16.4f, 3.0f}},
    {Seg::Cubic, {16.7f, 3.0f, 16.8f, 3.04f, 17.0f, 3.12f}},
    {Seg::Cubic, {17.2f, 3.21f, 17.4f, 3.32f, 17.5f, 3.45f}},
    {Seg::Line, {20.6f, 6.48f}},
    {Seg::Cubic, {20.7f, 6.61f, 20.8f, 6.77f, 20.9f, 6.96f}},
    {Seg::Cubic, {21.0f, 7.15f, 21.0f, 7.35f, 21.0f, 7.55f}},
    {Seg::Line, {21.0f, 19.5f}},
    {Seg::Cubic, {21.0f, 19.9f, 20.9f, 20.2f, 20.6f, 20.6f}},
    {Seg::Cubic, {20.2f, 20.9f, 19.9f, 21.0f, 19.5f, 21.0f}},
    {Seg::Line, {4.5f, 21.0f}},
    {Seg::Close, {}},
    {Seg::Move, {13.8f, 17.1f}},
    {Seg::Cubic, {14.3f, 16.6f, 14.6f, 16.0f, 14.6f, 15.3f}},
    {Seg::Cubic, {14.6f, 14.6f, 14.3f, 13.9f, 13.8f, 13.4f}},
    {Seg::Cubic, {13.3f, 12.9f, 12.7f, 12.7f, 12.0f, 12.7f}},
    {Seg::Cubic, {11.3f, 12.7f, 10.7f, 12.9f, 10.2f, 13.4f}},
    {Seg::Cubic, {9.65f, 13.9f, 9.4f, 14.6f, 9.4f, 15.3f}},
    {Seg::Cubic, {9.4f, 16.0f, 9.65f, 16.6f, 10.2f, 17.1f}},
    {Seg::Cubic, {10.7f, 17.6f, 11.3f, 17.9f, 12.0f, 17.9f}},
    {Seg::Cubic, {12.7f, 17.9f, 13.3f, 17.6f, 13.8f, 17.1f}},
    {Seg::Close, {}},
    {Seg::Move, {6.58f, 9.4f}},
    {Seg::Line, {14.0f, 9.4f}},
    {Seg::Cubic, {14.2f, 9.4f, 14.4f, 9.33f, 14.6f, 9.19f}},
    {Seg::Cubic, {14.7f, 9.05f, 14.8f, 8.87f, 14.8f, 8.65f}},
    {Seg::Line, {14.8f, 6.58f}},
    {Seg::Cubic, {14.8f, 6.36f, 14.7f, 6.18f, 14.6f, 6.04f}},
    {Seg::Cubic, {14.4f, 5.9f, 14.2f, 5.83f, 14.0f, 5.83f}},
    {Seg::Line, {6.58f, 5.83f}},
    {Seg::Cubic, {6.36f, 5.83f, 6.18f, 5.9f, 6.04f, 6.04f}},
    {Seg::Cubic, {5.9f, 6.18f, 5.83f, 6.36f, 5.83f, 6.58f}},
    {Seg::Line, {5.83f, 8.65f}},
    {Seg::Cubic, {5.83f, 8.87f, 5.9f, 9.05f, 6.04f, 9.19f}},
    {Seg::Cubic, {6.18f, 9.33f, 6.36f, 9.4f, 6.58f, 9.4f}},
    {Seg::Close, {}},
};
const Seg kCancelSegs0[] = {
    {Seg::Move, {12.0f, 13.1f}},
    {Seg::Line, {6.75f, 18.3f}},
    {Seg::Cubic, {6.6f, 18.4f, 6.43f, 18.5f, 6.23f, 18.5f}},
    {Seg::Cubic, {6.03f, 18.5f, 5.85f, 18.4f, 5.7f, 18.3f}},
    {Seg::Cubic, {5.55f, 18.2f, 5.48f, 18.0f, 5.48f, 17.8f}},
    {Seg::Cubic, {5.48f, 17.6f, 5.55f, 17.4f, 5.7f, 17.2f}},
    {Seg::Line, {11.0f, 12.0f}},
    {Seg::Line, {5.7f, 6.75f}},
    {Seg::Cubic, {5.55f, 6.6f, 5.48f, 6.43f, 5.48f, 6.23f}},
    {Seg::Cubic, {5.48f, 6.03f, 5.55f, 5.85f, 5.7f, 5.7f}},
    {Seg::Cubic, {5.85f, 5.55f, 6.03f, 5.48f, 6.23f, 5.48f}},
    {Seg::Cubic, {6.43f, 5.48f, 6.6f, 5.55f, 6.75f, 5.7f}},
    {Seg::Line, {12.0f, 11.0f}},
    {Seg::Line, {17.2f, 5.7f}},
    {Seg::Cubic, {17.4f, 5.55f, 17.6f, 5.48f, 17.8f, 5.48f}},
    {Seg::Cubic, {18.0f, 5.48f, 18.2f, 5.55f, 18.3f, 5.7f}},
    {Seg::Cubic, {18.4f, 5.85f, 18.5f, 6.03f, 18.5f, 6.23f}},
    {Seg::Cubic, {18.5f, 6.43f, 18.4f, 6.6f, 18.3f, 6.75f}},
    {Seg::Line, {13.1f, 12.0f}},
    {Seg::Line, {18.3f, 17.2f}},
    {Seg::Cubic, {18.4f, 17.4f, 18.5f, 17.6f, 18.5f, 17.8f}},
    {Seg::Cubic, {18.5f, 18.0f, 18.4f, 18.2f, 18.3f, 18.3f}},
    {Seg::Cubic, {18.2f, 18.4f, 18.0f, 18.5f, 17.8f, 18.5f}},
    {Seg::Cubic, {17.6f, 18.5f, 17.4f, 18.4f, 17.2f, 18.3f}},
    {Seg::Line, {12.0f, 13.1f}},
    {Seg::Close, {}},
};
const Seg kCheckSegs0[] = {
    {Seg::Move, {9.45f, 15.7f}},
    {Seg::Line, {18.5f, 6.62f}},
    {Seg::Cubic, {18.7f, 6.48f, 18.9f, 6.4f, 19.1f, 6.4f}},
    {Seg::Cubic, {19.3f, 6.4f, 19.5f, 6.48f, 19.6f, 6.62f}},
    {Seg::Cubic, {19.8f, 6.78f, 19.8f, 6.95f, 19.8f, 7.16f}},
    {Seg::Cubic, {19.8f, 7.37f, 19.8f, 7.55f, 19.6f, 7.7f}},
    {Seg::Line, {9.98f, 17.3f}},
    {Seg::Cubic, {9.83f, 17.5f, 9.65f, 17.6f, 9.45f, 17.6f}},
    {Seg::Cubic, {9.25f, 17.6f, 9.08f, 17.5f, 8.93f, 17.3f}},
    {Seg::Line, {4.38f, 12.8f}},
    {Seg::Cubic, {4.23f, 12.6f, 4.15f, 12.4f, 4.16f, 12.2f}},
    {Seg::Cubic, {4.17f, 12.0f, 4.25f, 11.9f, 4.4f, 11.7f}},
    {Seg::Cubic, {4.55f, 11.6f, 4.73f, 11.5f, 4.94f, 11.5f}},
    {Seg::Cubic, {5.15f, 11.5f, 5.33f, 11.6f, 5.48f, 11.7f}},
    {Seg::Line, {9.45f, 15.7f}},
    {Seg::Close, {}},
};

const GlyphPart kPenParts[] = {
    {kPenSegs0, 27, true}
};
const GlyphPart kRectangleParts[] = {
    {kRectangleSegs0, 24, true}
};
const GlyphPart kArrowParts[] = {
    {kArrowSegs0, 22, true}
};
const GlyphPart kTextParts[] = {
    {kTextSegs0, 38, true}
};
const GlyphPart kColorPickerParts[] = {
    {kColorPickerSegs0, 36, true}
};
const GlyphPart kUndoParts[] = {
    {kUndoSegs0, 34, true}
};
const GlyphPart kRedoParts[] = {
    {kRedoSegs0, 34, true}
};
const GlyphPart kOcrParts[] = {
    {kOcrSegs0, 114, true}
};
const GlyphPart kPinParts[] = {
    {kPinSegs0, 35, true}
};
const GlyphPart kRecordParts[] = {
    {kRecordSegs0, 22, true}
};
const GlyphPart kCopyParts[] = {
    {kCopySegs0, 40, true}
};
const GlyphPart kSaveParts[] = {
    {kSaveSegs0, 41, true}
};
const GlyphPart kCancelParts[] = {
    {kCancelSegs0, 26, true}
};
const GlyphPart kCheckParts[] = {
    {kCheckSegs0, 16, true}
};

Glyph glyphFor(ToolbarIcon icon)
{
    switch (icon) {
    case ToolbarIcon::Pen: return {kPenParts, 1};
    case ToolbarIcon::Rectangle: return {kRectangleParts, 1};
    case ToolbarIcon::Arrow: return {kArrowParts, 1};
    case ToolbarIcon::Text: return {kTextParts, 1};
    case ToolbarIcon::ColorPicker: return {kColorPickerParts, 1};
    case ToolbarIcon::Undo: return {kUndoParts, 1};
    case ToolbarIcon::Redo: return {kRedoParts, 1};
    case ToolbarIcon::Ocr: return {kOcrParts, 1};
    case ToolbarIcon::Pin: return {kPinParts, 1};
    case ToolbarIcon::Record: return {kRecordParts, 1};
    case ToolbarIcon::Copy: return {kCopyParts, 1};
    case ToolbarIcon::Save: return {kSaveParts, 1};
    case ToolbarIcon::Cancel: return {kCancelParts, 1};
    case ToolbarIcon::Check: return {kCheckParts, 1};
    }
    return {nullptr, 0};
}

} // namespace

QPixmap drawToolbarIconAt(
    ToolbarIcon icon, const QColor& color, int logicalSize, qreal devicePixelRatio)
{
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
    for (std::size_t part = 0; part < glyph.count; ++part) {
        const GlyphPart& glyphPart = glyph.parts[part];
        QPainterPath path;
        for (std::size_t i = 0; i < glyphPart.count; ++i) {
            const Seg& segment = glyphPart.segs[i];
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
        if (glyphPart.fill) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
        } else {
            painter.setPen(QPen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.setBrush(Qt::NoBrush);
        }
        painter.drawPath(path);
    }
    painter.end();
    return pixmap;
}

QPixmap drawToolbarIcon(ToolbarIcon icon, const QColor& color)
{
    return drawToolbarIconAt(icon, color, 24, 2.0);
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
    for (const int logicalSize : {20, 23}) {
        for (const qreal scaleFactor : {1.0, 1.25, 1.5, 2.0}) {
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
        }
    }
    return result;
}

} // namespace snipnexs
