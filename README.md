# SnipNexs

SnipNexs 是面向 Windows 10/11 的轻量截图工具，使用 C++20 与 Qt 6 Widgets 开发。

当前版本：`0.1.0`（阶段 1）。已具备可运行主窗口、系统托盘、单实例唤醒、命令行自检和独立部署能力。截图、标注、贴图、OCR、翻译与录屏将按阶段加入。

## 设计目标

- 轻量：不使用 QML、WebEngine 或 Chromium；未被实际功能需要的库不进入依赖。
- 高性能：图像数据尽量共享，耗时工作不阻塞 UI 线程，后续用测量结果决定是否引入原生加速。
- 可扩展：按业务能力分模块；只在 OCR、翻译、编码器等确实存在多实现的边界使用接口。
- Windows 10：最低目标为 Windows 10 2004（x64），主要验证环境为 Windows 10 22H2 与 Windows 11。

## 构建

要求：

- Visual Studio 2022 C++ 工具链
- CMake 3.25+
- Ninja
- Qt 6.8.x MSVC 2022 x64（动态库版本）

PowerShell：

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
