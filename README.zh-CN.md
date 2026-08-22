# SnipNexs

[English](README.md) | [简体中文](README.zh-CN.md)

SnipNexs 是面向 Windows 10/11 的轻量截图工具，使用 C++20 与 Qt 6 Widgets 开发。

当前版本：`0.2.0`（阶段 2）。已具备可运行主窗口、系统托盘、单实例唤醒、区域截图、复制/保存和独立部署能力。标注、贴图、OCR、翻译与录屏将按阶段加入。

## 使用

- 按 `Ctrl+Shift+A`，或点击主窗口的“区域截图”。
- 在鼠标所在显示器拖出选区。
- 按 `Enter` 或双击选区可复制；也可使用选区旁的“复制”“保存”按钮。
- 按 `Esc` 或右键取消。

## 设计目标

- 轻量：不使用 QML、WebEngine 或 Chromium；未被实际功能需要的库不进入依赖。
- 高性能：图像数据尽量共享，耗时工作不阻塞 UI 线程，后续用测量结果决定是否引入原生加速。
- 可扩展：按业务能力分模块；只在 OCR、翻译、编码器等确实存在多实现的边界使用接口。
- Windows 10：最低目标为 Windows 10 2004（x64），主要验证环境为 Windows 10 22H2 与 Windows 11。

当前技术边界见[架构说明](docs/architecture.md)和[依赖与许可](docs/dependencies.md)。

## 构建

要求：

- Visual Studio 2022 C++ 工具链
- CMake 3.25+
- Ninja
- Qt 6.8.x MSVC 2022 x64（动态库版本）

请在 x64 Visual Studio 开发环境中运行 PowerShell：

```powershell
$env:SNIPNEXS_QT_DIR = 'C:\Qt\6.8.3\msvc2022_64'
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
$installRoot = Join-Path $PWD 'dist\SnipNexs'
cmake --install build/release --prefix $installRoot
```

部署后的程序位于 `dist/SnipNexs/bin/SnipNexs.exe`。

## 许可证

SnipNexs 采用 [GNU GPL v3.0 or later](LICENSE)。Qt 以动态链接方式使用，遵循 Qt LGPL 的相关要求；发布包不得移除 Qt 的许可文本，且必须允许用户替换 Qt 动态库。

精确的发布组件和许可证来源见[第三方声明](THIRD_PARTY_NOTICES.md)。
