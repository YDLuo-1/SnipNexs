# SnipNexs v0.8.0

> [!IMPORTANT]
> Windows 10/11 x64 portable build. Extract the ZIP before running `bin/SnipNexs.exe`; do not run it inside the archive.
>
> Windows 10/11 x64 便携版。请先完整解压 ZIP，再运行 `bin/SnipNexs.exe`，不要直接在压缩包内启动。

## 📦 Download / 下载

| File / 文件 | Purpose / 用途 |
| --- | --- |
| `SnipNexs-0.8.0-win64.zip` | Portable application / 免安装便携应用 |

> [!TIP]
> Most users only need `SnipNexs-0.8.0-win64.zip`.
>
> 普通用户只需要下载 `SnipNexs-0.8.0-win64.zip`。

## English

### ✨ Highlights

- Reworked the capture toolbar as a compact light surface with 13 independently drawn monochrome icons. Text annotation and OCR now use clearly different glyphs; every action retains a tooltip and accessible name.
- Added inline single-line text annotation. Click inside the selection to type, press `Enter` to commit, press `Esc` to cancel, and use the existing undo/redo actions after committing.
- Added a fully local color picker with a 9 × 9 magnified pixel grid, physical pixel coordinates, RGB/HEX display, `C` to copy, `Shift` to switch formats, and click-to-copy.
- Moved physical pixel dimensions out of the toolbar into a floating label attached to the selection frame. Live and history captures preserve their own DPI and original physical size.
- Fixed high-DPI pinned-image rendering so the complete physical source image is displayed. Pinned images now keep the original source for copy/save, close on a left double-click, and expose a right-click menu for copy, save, toolbar visibility, and close.
- Added a hidden-by-default compact pin toolbar and a persistent local history beside `SnipNexs.exe` in `history`, capped at 20 PNGs/about 64 MiB and restored after restart. A non-writable application directory falls back to Windows local application data.

### ✅ Verification

- CTest: 14 tests, 0 failures; 12 passed and 2 skipped as designed.
- Regression coverage includes text composition and undo, RGB/HEX copying, distinct Text/OCR icons, 13 icon-only toolbar actions, DPI-aware selection dimensions, complete high-DPI pin rendering, pin menu gestures, and persistent history capacity/corruption recovery.
- Offscreen QA renders were generated for the toolbar and color-picker panel.

> [!WARNING]
> Windows desktop input automation was denied access on the validation machine. Real F1 capture interaction and mixed-DPI desktop behavior remain pending manual verification and are not counted as passed.

## 简体中文

### ✨ 主要更新

- 截图工具栏改为紧凑浅色面板，包含 13 个自主绘制的单色线性图标；“文字”和“识字”使用明确不同的图形，每个操作继续提供 Tooltip 和无障碍名称。
- 新增单行文字标注：在选区内单击后输入，按 `Enter` 完成、按 `Esc` 取消，完成后可沿用撤销和重做。
- 新增完全本地的取色器：显示 9 × 9 像素放大格、物理像素坐标和 RGB/HEX；按 `C` 复制、按 `Shift` 切换格式，单击可复制并退出。
- 物理像素尺寸从工具栏移到选框旁的浮层；实时截图与历史截图分别使用自己的 DPI 和原始物理尺寸。
- 修复高 DPI 贴图只显示左上区域的问题；贴图复制/保存使用原始图片，左键双击关闭，右键菜单提供复制、保存、显示/隐藏工具条和关闭。
- 新增默认隐藏的紧凑贴图工具条，并将历史持久化到程序目录下的 `history` 文件夹；最多保留 20 张 PNG、约 64 MiB，程序重启后恢复。程序目录不可写时回退到 Windows 本地应用数据目录。

### ✅ 验证结果

- CTest 共 14 项，零失败：12 项执行通过，2 项按设计跳过。
- 回归测试覆盖文字合成与撤销、RGB/HEX 复制、文字与识字图标差异、13 个纯图标按钮和 DPI 感知的选区尺寸。
- 已生成工具栏和取色面板的离屏 QA 渲染图。

> [!WARNING]
> 验证机拒绝桌面输入自动化访问，因此真实 F1 截图交互和混合 DPI 桌面行为仍待人工复核，未计为通过。

## 📜 Open-source compliance / 开源合规

- SnipNexs: GPL-3.0-or-later.
- Qt 6.11.2: LGPL-3.0-only, dynamically linked and replaceable.
- Qt corresponding source: [`qtbase-everywhere-src-6.11.2.tar.xz`](https://github.com/YDLuo-1/SnipNexs/releases/download/v0.7.0/qtbase-everywhere-src-6.11.2.tar.xz). The same unmodified Qt build is shared by v0.7.0 through v0.8.0.
- License texts, third-party notices, the Qt SBOM, and Qt DLL replacement instructions are included in the application ZIP.
- SnipNexs：GPL-3.0-or-later。
- Qt 6.11.2：LGPL-3.0-only，采用动态链接且允许替换 Qt DLL。
- Qt 对应源码：[`qtbase-everywhere-src-6.11.2.tar.xz`](https://github.com/YDLuo-1/SnipNexs/releases/download/v0.7.0/qtbase-everywhere-src-6.11.2.tar.xz)。v0.7.0 至 v0.8.0 共用同一份未修改 Qt 构建。
- 应用 ZIP 包含许可文本、第三方声明、Qt SBOM 和 Qt DLL 替换说明。

**Full Changelog / 完整变更：** [`v0.7.1...v0.8.0`](https://github.com/YDLuo-1/SnipNexs/compare/v0.7.1...v0.8.0)
