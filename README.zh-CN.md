# SnipNexs

[English](README.md) | [简体中文](README.zh-CN.md)

SnipNexs 是面向 Windows 10/11 的轻量截图与录屏工具，使用 C++20 与 Qt 6 Widgets 开发。

当前版本：`0.8.4`。已具备中英界面切换、托盘化启动、单实例唤醒、分层窗口识别、区域截图、截图历史、文字与图形标注、本地取色、可拖动缩放贴图、本地 Windows OCR、本地离线翻译、显式浏览器翻译、原生区域录屏、复制/保存和独立部署能力。

## 使用

- 有系统托盘时程序默认在后台启动，不显示主界面；双击托盘图标、选择“打开 SnipNexs”或再次运行程序可打开主界面。托盘不可用时自动显示主界面。
- 按 `F1`，或点击主窗口的“区域截图”。如果 F1 已被占用，SnipNexs 会回退到 `Ctrl+Shift+A`，并显示实际生效的快捷键。
- 可在主窗口选择“简体中文”或“English”；切换立即生效，并在下次启动时保持。
- 鼠标悬停桌面窗口时会依次预览原生子窗口、客户区或整窗边界，单击即可选中；没有独立原生窗口的视觉元素会回退到所属客户区或整窗。拖动则创建自定义选区。开始标注前，可拖动选区内部来移动，也可拖动四周八个控制点缩放。
- 截图期间主窗口保持隐藏；成功复制、保存或贴出的截图会写入 `SnipNexs.exe` 同目录下的 `history` 文件夹，按 `,` 查看上一张、按 `.` 查看下一张。历史最多保留 20 张原始 PNG、约 64 MiB，程序重启后仍可恢复。如果程序目录不可写（例如安装在 `Program Files`），会回退到 Windows 本地应用数据目录。
- 选框旁会显示物理像素尺寸。可用“画笔”“矩形”“箭头”“文字”标注，“马赛克”“模糊”笔刷可在分享或翻译前遮挡敏感信息；文字输入按 `Enter` 完成、按 `Esc` 取消，`Ctrl+Z` 与 `Ctrl+Y` 撤销和重做。
- 点击“取色”可查看 9 × 9 像素放大格；按 `C` 复制颜色，按 `Shift` 切换 RGB/HEX，或单击颜色后复制并退出取色。
- 点击“识字”可使用已安装的 Windows OCR 语言包在本机识别选区。
- 识别结果可点击“本地翻译”，统一使用腾讯 Hy-MT2-1.8B 高质量引擎（llama.cpp CPU 推理），不再提供快速引擎或引擎切换。约 1.1 GB 模型经 SHA-256 校验后可离线使用，本地离线包已预装。CPU 推理速度取决于硬件和文本长度，机器翻译仍需核对。多行文本可使用“对照”视图逐行查看原文与译文。选型依据见[本地翻译选型决策](docs/local-translation-decision.md)。
- 结果窗口可复制文字，或在浏览器中翻译成中文/英文；只有确认后才发送文字，不发送图片。
- 点击主窗口“区域录屏”，拖出区域后点击选区工具栏的“录屏”；选择 MP4 路径，再用悬浮条停止。悬浮条默认位于所选显示器右下角，拖动空白处可移动。
- 录屏包含鼠标指针，通过 Windows 媒体 API 写入 H.264/MP4 与 AAC 音频；默认录制系统声音（回路），可在设置对话框中切换为麦克风或关闭（`recording/audio`）。
- 按 `Enter` 或双击选区可复制；也可使用选区旁的“贴图”“复制”“保存”按钮。
- 贴图保持置顶：拖动可移动，滚轮以鼠标所指内容为锚点连续缩放，双击左键关闭；单击右键打开“复制图像”“图像另存为”“显示/隐藏工具条”“关闭贴图”菜单。紧凑工具条默认隐藏，可从菜单显示。
- 主界面底部和托盘菜单均可打开“关于 SnipNexs”，离线查看程序、Qt 与录屏参考代码的许可证及源码位置。
- 主界面底部和托盘菜单可打开“设置…”：切换录屏音频（系统声音/麦克风/无）、设置翻译模型镜像 URL、管理已安装的语言包（查看与删除）以及检查更新。启动时自动检查更新为可选项，仅在你开启后才访问 GitHub，不上传任何数据。
- 按 `Esc` 或右键取消。

当前开发树的稳定性改进：无托盘时使用普通最小化；修改 OCR 文字或切换翻译引擎会清除旧译文；历史记录在后台保存，正常退出时完成待保存记录。录屏期间正常退出也会完成 MP4 保存；若编码或保存失败，非空的 `.partial.mp4` 保留在目标文件旁以便排查，不能保证未完成文件一定可播放。0.8.4 还增加了有界模型下载、高质量翻译的 CPU 后端自动选择，以及 Windows Graphics Capture 不可用时的区域录屏兼容路径。

## 隐私

- 除非你明确选择“复制”“保存”或“贴图”，屏幕像素只存在于本地进程内存。
- 当前版本没有遥测；OCR 完全在本机执行，不会上传截图或标注数据。
- 本地翻译同样完全在本机执行：文字、模型与翻译结果都不出设备，语言包从 GitHub Release 下载并经 SHA-256 校验。
- 录屏完全在本机执行；帧只写入你选择的路径，不直播也不上传视频。
- 浏览器翻译是主动操作：确认后，最多 4000 个识别字符会放入 `translate.google.com` URL，不会传输图片。
- 更新检查同样为主动操作：只有点击“立即检查”或开启启动时检查后才会读取 GitHub 的最新版本号，不上传任何数据。
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

- 捕获：优先使用 `Windows.Graphics.Capture`；不可用或系统拒绝访问时，通过 GDI 仅采集所选区域。
- 裁剪：WGC 路径使用 D3D11 `CopySubresourceRegion`，不做 CPU 回读；兼容路径使用选区大小的 DIB，再上传至 D3D11。
- 编码与封装：`MediaStreamSource`、`MediaTranscoder`，输出 H.264/MP4。WGC 路径优先请求硬件编码，准备失败时尝试软件编码；兼容捕获使用软件编码。音视频统一使用从零开始的 QPC 时间轴。
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

如果仓库被移动或改名，请用 `cmake --fresh --preset windows-release` 重新配置（或删除 `build/release`）；CMake 与 Ninja 会保存绝对源码路径。

正式发布必须在干净且已打对应版本标签的提交上运行：

```powershell
.\scripts\package-release.ps1 -Version 0.8.4
```

脚本会重新构建、通过 CTest、部署 Qt 与 MSVC 应用本地运行库、复制许可和 Qt SBOM、验证部署程序、生成 ZIP，并输出其 SHA-256；GitHub 也会为每个上传附件直接显示摘要。仅当某个 Qt 版本第一次用于 SnipNexs 发布时才添加 `-IncludeQtSource`；后续应用版本统一链接到这份由发布者控制的源码副本。发布验收要求见[发布清单](docs/release-checklist.md)。

## 许可证

SnipNexs 采用 [GNU GPL v3.0 or later](LICENSE)。Qt 6.11.2 以动态链接方式使用，遵循 LGPL-3.0-only；发布包保留许可、SBOM、对应源码副本位置和替换 Qt DLL 的方法。

精确的发布组件和许可证来源见[第三方声明](THIRD_PARTY_NOTICES.md)。

### 本地修复候选包

```powershell
.\scripts\package-release.ps1 -Version 0.8.4 -LocalCandidate -ModelSource .\build\models\src
```

`-LocalCandidate` 允许从开发树生成本地候选包；默认正式打包仍要求工作树干净且 HEAD 带对应标签。`-ModelSource` 可选，按源码清单校验并打入唯一的 Hy-MT2 离线模型。打包始终使用新暂存目录，校验 ZIP 后才清理旧部署。历史与模型迁移到新版；其他新增或修改文件保留在项目 `user-data/<旧部署名>`，已有 ZIP 留作回退。目标版本已存在时会停止，避免覆盖正在使用的目录。安装器仅打入程序与许可文件，模型由应用按需下载。

OCR 翻译方向按当前文字内容推断，也可明确选择“译为中文/英文”。翻译可取消，关闭结果窗口也会取消其任务。引擎按 token 限制分段；输出未完成时显示错误，不把截断结果当作成功。

本地翻译统一使用 Hy-MT2，不保存引擎选择设置。打包脚本保留旧快速模型到 `user-data/<旧部署名>`，不会把它们带入新包。
