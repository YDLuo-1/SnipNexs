# SnipNexs

[English](README.md) | [简体中文](README.zh-CN.md)

SnipNexs is a lightweight screenshot toolkit for Windows 10/11, built with C++20 and Qt 6 Widgets.

Current version: `0.4.0` (Stage 4). It includes a runnable main window, system tray integration, single-instance activation, region capture, lightweight annotation, image pinning, local Windows OCR, explicit browser translation, copy/save actions, and standalone deployment. Recording will be added in a later stage.

## Usage

- Press `Ctrl+Shift+A`, or click **Region Capture** in the main window.
- Drag a region on the monitor under the mouse pointer.
- Use **Pen**, **Rectangle**, or **Arrow** to annotate. `Ctrl+Z` and `Ctrl+Y` undo and redo.
- Use **OCR** to recognize the selected image locally with an installed Windows OCR language pack.
- The result window can copy text or open a Chinese/English browser translation. Translation sends text only after a confirmation dialog; it never sends the image.
- Press `Enter` or double-click the selection to copy it, or use **Pin**, **Copy**, and **Save** next to the selection.
- Pinned images stay on top: drag to move, use the mouse wheel to resize, and right-click to close.
- Press `Esc` or right-click to cancel.

## Privacy

- Screen pixels stay in local process memory unless you explicitly choose **Copy**, **Save**, or **Pin**.
- The current version has no telemetry. OCR is local and never uploads screenshots or annotation data.
- Browser translation is opt-in: after confirmation, at most 4,000 recognized characters are placed in a `translate.google.com` URL. No image is transmitted.
- Generated `SnipNexs-*` images and common capture directories are ignored by Git. Every stage is also audited for staged images, binaries, local paths, and likely secrets before push.

## Design goals

- Lightweight: no QML, WebEngine, or Chromium; a dependency is added only when a delivered feature requires it.
- Fast: image data is shared where possible, expensive work stays off the UI thread, and native acceleration is introduced only after measurement.
- Extensible: code is separated by capability; interfaces are used only at boundaries that genuinely need multiple implementations, such as OCR, translation, or encoders.
- Windows 10: the minimum target is Windows 10 2004 (x64); primary validation targets are Windows 10 22H2 and Windows 11.

See [Architecture](docs/architecture.md), [Dependencies and licenses](docs/dependencies.md), and the pinned [source review](docs/source-review.md) for the current technical boundaries.

## Build

Requirements:

- Visual Studio 2022 C++ toolchain with Windows SDK `10.0.22621.0` or newer (C++/WinRT included)
- CMake 3.25+
- Ninja
- Qt 6.8.x for MSVC 2022 x64, shared-library build

PowerShell, from an x64 Visual Studio developer environment:

```powershell
$env:SNIPNEXS_QT_DIR = 'C:\Qt\6.8.3\msvc2022_64'
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
$installRoot = Join-Path $PWD 'dist\SnipNexs'
cmake --install build/release --prefix $installRoot
```

The deployed executable is `dist/SnipNexs/bin/SnipNexs.exe`.

## License

SnipNexs is licensed under the [GNU GPL v3.0 or later](LICENSE). Qt is dynamically linked under its LGPL terms. Distributed packages must retain the applicable Qt license notices and allow users to replace the Qt DLLs.

See [Third-party notices](THIRD_PARTY_NOTICES.md) for exact deployed components and license sources.
