# SnipNexs

[English](README.md) | [简体中文](README.zh-CN.md)

SnipNexs is a lightweight screenshot and recording toolkit for Windows 10/11, built with C++20 and Qt 6 Widgets.

Current version: `0.8.0`. It includes Chinese/English UI switching, tray-first startup, single-instance activation, hierarchical window targeting, region capture and session history, text and shape annotation, a local color picker, draggable and resizable image pinning, local Windows OCR, explicit browser translation, native GPU-backed region recording, copy/save actions, and standalone deployment.

## Usage

- When a system tray is available, SnipNexs starts in the background without showing the main window. Double-click the tray icon, choose **Open SnipNexs**, or launch the program again to open it. The main window is shown automatically if no tray is available.
- Press `F1`, or click **Region Capture** in the main window. If F1 is already in use, SnipNexs falls back to `Ctrl+Shift+A` and shows the active shortcut.
- Choose **Simplified Chinese** or **English** from the language selector. The change takes effect immediately and is remembered for the next launch.
- Hover a desktop window to preview a native child window, client area, or whole-window frame, then click to select it; drag instead to create a custom region. Visual elements without their own native window still fall back to the surrounding client area or window frame. Before annotating, drag inside the selection to move it, or drag any of its eight handles to resize it.
- The main window stays hidden during capture. Successfully copied, saved, or pinned captures are written to the local history at `%LOCALAPPDATA%/SnipNexs/history`; press `,` for the previous image and `.` for the next one. The history keeps at most 20 original PNGs and about 64 MiB, and is restored after restart.
- The selection frame shows its physical pixel dimensions. Use **Pen**, **Rectangle**, **Arrow**, or **Text** to annotate; press `Enter` to commit inline text and `Esc` to cancel it. `Ctrl+Z` and `Ctrl+Y` undo and redo.
- Use **Color picker** to inspect a 9 × 9 magnified pixel grid. Press `C` to copy the value, `Shift` to toggle RGB/HEX, or click to copy and close the picker.
- Use **OCR** to recognize the selected image locally with an installed Windows OCR language pack.
- The result window can copy text or open a Chinese/English browser translation. Translation sends text only after a confirmation dialog; it never sends the image.
- Click **Region Recording**, draw a region, then click **Record** in the selection toolbar. Choose an MP4 path and use the floating indicator to stop. The indicator starts at the bottom-right of the selected display and can be dragged from an empty area.
- Recording captures the pointer and writes H.264/MP4 through Windows media APIs. Recording is currently video-only; system audio and microphone input are not included yet.
- Press `Enter` or double-click the selection to copy it, or use **Pin**, **Copy**, and **Save** next to the selection.
- Pinned images stay on top: drag to move, use the mouse wheel for cumulative cursor-anchored resizing, double-click the left button to close, and right-click for Copy Image, Save Image As, toolbar visibility, or Close Pin. The compact pin toolbar is hidden by default and can be shown from that menu.
- Open **About SnipNexs** from the main-window footer or tray menu to read the application, Qt, and recording-reference licenses and source locations offline.
- Press `Esc` or right-click to cancel.

## Privacy

- Screen pixels stay in local process memory unless you explicitly choose **Copy**, **Save**, or **Pin**.
- The current version has no telemetry. OCR is local and never uploads screenshots or annotation data.
- Recording is local. Frames stay on the GPU path and are written only to the path you choose; SnipNexs does not stream or upload the video.
- Browser translation is opt-in: after confirmation, at most 4,000 recognized characters are placed in a `translate.google.com` URL. No image is transmitted.
- Generated `SnipNexs-*` images and common capture directories are ignored by Git. Every stage is also audited for staged images, binaries, local paths, and likely secrets before push.

## Design goals

- Lightweight: no QML, WebEngine, or Chromium; a dependency is added only when a delivered feature requires it.
- Fast: image data is shared where possible, expensive work stays off the UI thread, and recording uses a two-frame Windows capture pool with a single latest-frame slot instead of an unbounded queue.
- Extensible: code is separated by capability; interfaces are used only at boundaries that genuinely need multiple implementations, such as OCR, translation, or encoders.
- Windows 10: the minimum target is Windows 10 2004 (x64); primary validation targets are Windows 10 22H2 and Windows 11.

See [Architecture](docs/architecture.md), [Dependencies and licenses](docs/dependencies.md), and the pinned [source review](docs/source-review.md) for the current technical boundaries.

## Open source, development, and use notice

Generative AI tools assisted with parts of the requirements analysis, design, documentation, code, and testing. The [open source, development, and use notice](docs/ai-assisted-development.md) explains the GPL and third-party license scope, AI-assisted development responsibilities, testing boundaries, warranty terms, privacy precautions, and feedback channel.

## Recording implementation notes

- Capture: `Windows.Graphics.Capture`, supported by the project minimum of Windows 10 2004.
- Crop: D3D11 `CopySubresourceRegion`; screen pixels are not read back to CPU memory.
- Encode/container: `MediaStreamSource` and `MediaTranscoder`, H.264 in MP4. Hardware acceleration is requested, but the final encoder choice still depends on the installed driver and Windows codecs.
- Backpressure: the capture pool has two buffers and SnipNexs retains only the newest frame; the transcoder requests frames as it can consume them.
- The floating stop indicator requests `WDA_EXCLUDEFROMCAPTURE`. Windows treats this as a best-effort display-affinity feature, not a security boundary.

## Build

Requirements:

- Visual Studio 2022 C++ toolchain with Windows SDK `10.0.22621.0` or newer (C++/WinRT included)
- CMake 3.25+
- Ninja
- Qt 6.11.2 for MSVC 2022 x64, shared-library build

For a development build, use PowerShell from an x64 Visual Studio developer environment:

```powershell
$env:SNIPNEXS_QT_DIR = 'D:\Qt\6.11.2\msvc2022_64'
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
$installRoot = Join-Path $PWD 'dist\SnipNexs'
cmake --install build/release --prefix $installRoot
```

The deployed executable is `dist/SnipNexs/bin/SnipNexs.exe`.

For a formal release, run this only from a clean commit carrying the matching version tag:

```powershell
.\scripts\package-release.ps1 -Version 0.8.0
```

The script rebuilds, runs CTest, deploys Qt and the app-local MSVC runtime, copies notices and the Qt SBOM, checks the deployed executable, creates the ZIP, and prints its SHA-256. GitHub also displays the digest for every uploaded asset. Add `-IncludeQtSource` only when publishing the first SnipNexs release for a new Qt version; later application releases link to that publisher-controlled source copy. See the [release checklist](docs/release-checklist.md).

## License

SnipNexs is licensed under the [GNU GPL v3.0 or later](LICENSE). Qt 6.11.2 is dynamically linked under LGPL-3.0-only. Release packages retain the license, SBOM, corresponding-source location, and instructions for replacing the Qt DLLs.

See [Third-party notices](THIRD_PARTY_NOTICES.md) for exact deployed components and license sources.
