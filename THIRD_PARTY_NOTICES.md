# Third-party notices

## Qt 6.11.2

SnipNexs dynamically links the Qt Core, Gui, Widgets, and Network modules and deploys the required Qt platform, style, and image-format plugins.

- Project: <https://www.qt.io/>
- Corresponding source copy for v0.6.0: <https://github.com/YDLuo-1/SnipNexs/releases/download/v0.6.0/qtbase-everywhere-src-6.11.2.tar.xz>
- Source archive SHA-256: `5b2e00eccaf5a4d8c14134ffa0ea8dfd0a35ae1ffc7f8d87fa4305a1ed23cf22`
- License used by this distribution: GNU Lesser General Public License version 3
- License text: [licenses/LGPL-3.0-only.txt](licenses/LGPL-3.0-only.txt)
- Human-readable bundled-component notice: [licenses/QT-THIRD-PARTY-NOTICES-6.11.2.md](licenses/QT-THIRD-PARTY-NOTICES-6.11.2.md)
- Machine-readable component and attribution inventory: `licenses/qtbase-6.11.2.spdx`

The Qt DLLs are separate shared libraries. SnipNexs does not modify Qt and does not use Qt WebEngine, QML, or statically linked Qt libraries. To use a modified Qt build, exit SnipNexs, replace the Qt DLLs in `bin` and the matching files under `plugins` with an ABI-compatible Qt 6.11.2 shared build, and keep the directory structure and `bin/qt.conf`. SnipNexs applies no signature check or technical restriction to replacement. A distributor who ships modified Qt libraries must provide the corresponding modified Qt source and retain the applicable notices.

## Microsoft SimpleRecorder sample

The Windows recording pipeline design was informed by Microsoft's SimpleRecorder sample. SnipNexs does not distribute the sample binaries or its SharpDX dependency.

- Project: <https://github.com/MicrosoftDocs/SimpleRecorder>
- Reviewed source revision: `e6ec15684e4c51abca518b4cadd2d89d2b359509`
- Code license: MIT License
- License text: [licenses/MIT-Microsoft-SimpleRecorder.txt](licenses/MIT-Microsoft-SimpleRecorder.txt)

## Microsoft Visual C++ runtime

The portable ZIP includes application-local release CRT DLLs copied only from the Visual Studio 2022 `VC/Redist/MSVC/.../x64/Microsoft.VC143.CRT` directory. This avoids an administrator-level prerequisite installation. These Microsoft files are distributed under the applicable Visual Studio license terms and are not part of SnipNexs or covered by its GPL license.
