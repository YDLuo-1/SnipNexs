# SnipNexs

[English](README.md) | [简体中文](README.zh-CN.md)

SnipNexs 是面向 Windows 10/11 的轻量截图与录屏工具，使用 C++20 与 Qt 6 Widgets 开发。

当前版本：`0.8.0`。已具备中英界面切换、托盘化启动、单实例唤醒、分层窗口识别、区域截图、截图历史、文字与图形标注、本地取色、可拖动缩放贴图、本地 Windows OCR、显式浏览器翻译、原生 GPU 区域录屏、复制/保存和独立部署能力。

## 使用

- 有系统托盘时程序默认在后台启动，不显示主界面；双击托盘图标、选择“打开 SnipNexs”或再次运行程序可打开主界面。托盘不可用时自动显示主界面。
- 按 `F1`，或点击主窗口的“区域截图”。如果 F1 已被占用，SnipNexs 会回退到 `Ctrl+Shift+A`，并显示实际生效的快捷键。
- 可在主窗口选择“简体中文”或“English”；切换立即生效，并在下次启动时保持。
- 鼠标悬停桌面窗口时会依次预览原生子窗口、客户区或整窗边界，单击即可选中；没有独立原生窗口的视觉元素会回退到所属客户区或整窗。拖动则创建自定义选区。开始标注前，可拖动选区内部来移动，也可拖动四周八个控制点缩放。
- 截图期间主窗口保持隐藏；成功复制、保存或贴出的截图会进入本次运行的历史，按 `,` 查看上一张、按 `.` 查看下一张。
- 选框旁会显示物理像素尺寸。可用“画笔”“矩形”“箭头”“文字”标注；文字输入按 `Enter` 完成、按 `Esc` 取消，`Ctrl+Z` 与 `Ctrl+Y` 撤销和重做。
- 点击“取色”可查看 9 × 9 像素放大格；按 `C` 复制颜色，按 `Shift` 切换 RGB/HEX，或单击颜色后复制并退出取色。
- 点击“识字”可使用已安装的 Windows OCR 语言包在本机识别选区。
- 结果窗口可复制文字，或在浏览器中翻译成中文/英文；只有确认后才发送文字，不发送图片。
- 点击主窗口“区域录屏”，拖出区域后点击选区工具栏的“录屏”；选择 MP4 路径，再用悬浮条停止。悬浮条默认位于所选显示器右下角，拖动空白处可移动。
- 录屏包含鼠标指针，通过 Windows 媒体 API 写入 H.264/MP4。当前只录视频，暂不包含系统声音或麦克风。
- 按 `Enter` 或双击选区可复制；也可使用选区旁的“贴图”“复制”“保存”按钮。
- 贴图保持置顶：拖动可移动，滚轮以鼠标所指内容为锚点连续缩放，右键可关闭。
- 主界面底部和托盘菜单均可打开“关于 SnipNexs”，离线查看程序、Qt 与录屏参考代码的许可证及源码位置。
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

## 开源、开发与使用声明

本项目在部分需求分析、设计、文档、代码和测试工作中使用了生成式 AI 辅助。[开源、开发与使用声明](docs/ai-assisted-development.md)说明了 GPL 与第三方许可范围、AI 辅助开发责任、测试边界、无担保条款、隐私注意事项和反馈渠道。

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
- Qt 6.11.2 MSVC 2022 x64（动态库版本）

开发构建请在 x64 Visual Studio 开发环境中运行 PowerShell：

```powershell
$env:SNIPNEXS_QT_DIR = 'D:\Qt\6.11.2\msvc2022_64'
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
$installRoot = Join-Path $PWD 'dist\SnipNexs'
cmake --install build/release --prefix $installRoot
```

部署后的程序位于 `dist/SnipNexs/bin/SnipNexs.exe`。

正式发布必须在干净且已打对应版本标签的提交上运行：

```powershell
.\scripts\package-release.ps1 -Version 0.8.0
```

脚本会重新构建、通过 CTest、部署 Qt 与 MSVC 应用本地运行库、复制许可和 Qt SBOM、验证部署程序、生成 ZIP，并输出其 SHA-256；GitHub 也会为每个上传附件直接显示摘要。仅当某个 Qt 版本第一次用于 SnipNexs 发布时才添加 `-IncludeQtSource`；后续应用版本统一链接到这份由发布者控制的源码副本。发布验收要求见[发布清单](docs/release-checklist.md)。

## 许可证

SnipNexs 采用 [GNU GPL v3.0 or later](LICENSE)。Qt 6.11.2 以动态链接方式使用，遵循 LGPL-3.0-only；发布包保留许可、SBOM、对应源码副本位置和替换 Qt DLL 的方法。

精确的发布组件和许可证来源见[第三方声明](THIRD_PARTY_NOTICES.md)。
