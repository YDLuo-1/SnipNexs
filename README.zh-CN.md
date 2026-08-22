# SnipNexs

[English](README.md) | [简体中文](README.zh-CN.md)

SnipNexs 是面向 Windows 10/11 的轻量截图与录屏工具，使用 C++20 与 Qt 6 Widgets 开发。

当前版本：`0.5.0`（阶段 5）。已具备可运行主窗口、系统托盘、单实例唤醒、区域截图、轻量标注、贴图、本地 Windows OCR、显式浏览器翻译、原生 GPU 区域录屏、复制/保存和独立部署能力。

## 使用

- 按 `Ctrl+Shift+A`，或点击主窗口的“区域截图”。
- 在鼠标所在显示器拖出选区。
- 可用“画笔”“矩形”“箭头”标注；`Ctrl+Z` 与 `Ctrl+Y` 撤销和重做。
- 点击“识字”可使用已安装的 Windows OCR 语言包在本机识别选区。
- 结果窗口可复制文字，或在浏览器中翻译成中文/英文；只有确认后才发送文字，不发送图片。
- 点击主窗口“区域录屏”，拖出区域后点击选区工具栏的“录屏”；选择 MP4 路径，再用悬浮条停止。
- 录屏包含鼠标指针，通过 Windows 媒体 API 写入 H.264/MP4。阶段 5 只录视频，暂不包含系统声音或麦克风。
- 按 `Enter` 或双击选区可复制；也可使用选区旁的“贴图”“复制”“保存”按钮。
- 贴图保持置顶：拖动可移动，滚轮可缩放，右键可关闭。
- 按 `Esc` 或右键取消。

## 隐私

- 除非你明确选择“复制”“保存”或“贴图”，屏幕像素只存在于本地进程内存。
- 当前版本没有遥测；OCR 完全在本机执行，不会上传截图或标注数据。
- 录屏完全在本机执行；帧只写入你选择的路径，不直播也不上传视频。
- 浏览器翻译是主动操作：确认后，最多 4000 个识别字符会放入 `translate.google.com` URL，不会传输图片。
- Git 已忽略 `SnipNexs-*` 图片和常见截图目录；每个阶段推送前还会检查暂存图片、二进制、本机路径和疑似密钥。

## 设计目标

- 轻量：不使用 QML、WebEngine 或 Chromium；未被实际功能需要的库不进入依赖。
- 高性能：图像数据尽量共享，耗时工作不阻塞 UI 线程；录屏使用两个 Windows 捕获缓冲和一个“最新帧”槽位，不建立无界队列。
- 可扩展：按业务能力分模块；只在 OCR、翻译、编码器等确实存在多实现的边界使用接口。
- Windows 10：最低目标为 Windows 10 2004（x64），主要验证环境为 Windows 10 22H2 与 Windows 11。

当前技术边界见[架构说明](docs/architecture.md)、[依赖与许可](docs/dependencies.md)和固定提交号的[同类源码核对](docs/source-review.md)。

## 录屏实现说明

- 捕获：`Windows.Graphics.Capture`，项目最低支持的 Windows 10 2004 已包含该能力。
- 裁剪：D3D11 `CopySubresourceRegion`，屏幕像素不回读到 CPU 内存。
- 编码与封装：`MediaStreamSource`、`MediaTranscoder`，输出 H.264/MP4。程序会请求硬件加速，但最终编码器仍由显卡驱动和 Windows 编解码器决定。
- 背压：系统捕获池只有两个缓冲，SnipNexs 只保留最新一帧；编码器按自身消费能力取帧。
- 悬浮停止条请求 `WDA_EXCLUDEFROMCAPTURE`；它是 Windows 的尽力而为显示属性，不是安全边界。

## 构建

要求：

- Visual Studio 2022 C++ 工具链，以及 Windows SDK `10.0.22621.0` 或更高版本（包含 C++/WinRT）
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
