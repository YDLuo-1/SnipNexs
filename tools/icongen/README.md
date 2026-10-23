# Icon generation pipeline

The toolbar glyphs in `src/capture/ToolbarIcons.cpp` are **generated** from
the Material Symbols Rounded SVG sources in `material-sources/` (Apache-2.0).
The generated file is committed, so building never requires Python — run this
pipeline only when changing which glyph maps to which tool.

## Files

- `material-sources/*.svg` — vendored SVG sources on the 960-unit grid; each
  glyph keeps the weight variant actually used (`default` outlined or
  `fill1` solid)
- `fetch-material.py` — re-downloads the sources from the gstatic
  short-term release endpoint (only needed when adopting a different glyph)
- `gen_icons.py` — SVG path parser library (arc→cubic conversion, viewBox
  normalization) used by `gen_material.py`; it has no standalone entry point
- `gen_material.py` — parses the vendored sources and emits the segment
  tables + parts-based renderer in `src/capture/ToolbarIcons.cpp`
- `gen_app_icon.ps1` — renders `resources/app.ico` with the same geometry as
  `src/app/AppIcon.cpp` (Windows `System.Drawing`); `resources/SnipNexs.rc.in`
  embeds it together with the generated VERSIONINFO

## Application icon

```powershell
pwsh tools/icongen/gen_app_icon.ps1
```

This refreshes `resources/app.ico` (16–256 px). CMake generates the executable
icon, VERSIONINFO and custom manifest identity from
`resources/SnipNexs.rc.in`.

## Regenerating

```powershell
python tools/icongen/gen_material.py
cmake --build build/release
ctest --test-dir build/release -R ToolbarIconsTests
```

`gen_material.py` resolves `material-sources/` relative to its own location,
writes `src/capture/ToolbarIcons.cpp` in place, and is deterministic: after a
regeneration, `git diff --exit-code src/capture/ToolbarIcons.cpp` should be
clean. Commit the generated file together with any `material-sources/` changes.
The slot→glyph mapping lives in the `FINAL` dict at the top of
`gen_material.py` (tools use the fill1 solid weight, geometric shapes the
outlined weight — the Snipaste-like mixed language).

Note: the upstream endpoint is a rolling release; the vendored SVGs are the
pinned truth. Re-running `fetch-material.py` may silently change glyph
outlines — diff `material-sources/` before regenerating.
