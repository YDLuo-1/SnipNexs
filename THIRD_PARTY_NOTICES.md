# Third-party notices

## Qt 6.8.3

SnipNexs dynamically links the Qt Core, Gui, Widgets, and Network modules and deploys the required Qt platform, style, and image-format plugins.

- Project: <https://www.qt.io/>
- Source code: <https://download.qt.io/archive/qt/6.8/6.8.3/submodules/>
- License used by this distribution: GNU Lesser General Public License version 3
- License text: [licenses/LGPL-3.0-only.txt](licenses/LGPL-3.0-only.txt)

The Qt DLLs are separate shared libraries. Users may replace them with a compatible modified build. SnipNexs does not use Qt WebEngine, QML, or statically linked Qt libraries.

## Microsoft SimpleRecorder sample

The Windows recording pipeline design was informed by Microsoft's SimpleRecorder sample. SnipNexs does not distribute the sample binaries or its SharpDX dependency.

- Project: <https://github.com/MicrosoftDocs/SimpleRecorder>
- Reviewed source revision: `e6ec15684e4c51abca518b4cadd2d89d2b359509`
- Code license: MIT License
- License text: [licenses/MIT-Microsoft-SimpleRecorder.txt](licenses/MIT-Microsoft-SimpleRecorder.txt)

## Microsoft Visual C++ Redistributable

Windows deployment may include the Microsoft Visual C++ Redistributable installer produced by Qt's deployment tool. It is distributed under the applicable Microsoft Visual Studio license terms and is not part of SnipNexs.
