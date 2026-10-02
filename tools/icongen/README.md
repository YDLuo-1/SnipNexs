# Icon generation pipeline

The toolbar glyphs in `src/capture/ToolbarIcons.cpp` are **generated** from
the Material Symbols Rounded SVG sources in `material-sources/` (Apache-2.0).
The generated file is committed, so building never requires Python — run this
pipeline only when changing which glyph maps to which tool.

## Files

- `material-sources/*.svg` — vendored SVG sources (960-unit grid, both the
  default outlined weight and the fill1 solid weight per glyph)
- `fetch-material.py` — re-downloads the sources from the gstatic
  short-term release endpoint (only needed when adopting a different glyph)
- `gen_icons.py` — generic SVG path parser (arc→cubic conversion, viewBox
  normalization) used by the generator
- `gen_material.py` — parses the vendored sources and emits the segment
  tables + parts-based renderer in `src/capture/ToolbarIcons.cpp`

## Regenerating

```powershell
python tools/icongen/gen_material.py
cmake --build build/release
ctest --test-dir build/release -R ToolbarIconsTests
```

`gen_material.py` writes `src/capture/ToolbarIcons.cpp` in place; commit it
together with any `material-sources/` changes. The slot→glyph mapping lives
in the `FINAL` dict at the top of `gen_material.py` (tools use the fill1
solid weight, geometric shapes the outlined weight — the Snipaste-like mixed
language).

Note: the upstream endpoint is a rolling release; the vendored SVGs are the
pinned truth. Re-running `fetch-material.py` may silently change glyph
outlines — diff `material-sources/` before regenerating.
