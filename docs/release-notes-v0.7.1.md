# SnipNexs v0.7.1

> [!IMPORTANT]
> Windows 10/11 x64 portable build. Extract the ZIP before running `bin/SnipNexs.exe`; do not run it inside the archive.
>
> Windows 10/11 x64 便携版。请先完整解压 ZIP，再运行 `bin/SnipNexs.exe`，不要直接在压缩包内启动。

## 📦 Download / 下载

| File / 文件 | Purpose / 用途 |
| --- | --- |
| `SnipNexs-0.7.1-win64.zip` | Portable application / 免安装便携应用 |

> [!TIP]
> Most users only need `SnipNexs-0.7.1-win64.zip`.
>
> 普通用户只需要下载 `SnipNexs-0.7.1-win64.zip`。
>
> GitHub displays the ZIP's SHA-256 digest directly beside the asset. / GitHub 会在附件旁直接显示 ZIP 的 SHA-256 摘要。

## English

### ✨ Highlights

- Replaced all 11 capture-toolbar glyphs with a consistent, independently drawn icon set. Pin, Copy, Save, OCR, Record, annotation, history actions, and Cancel remain icon-only buttons with tooltips and accessible names.
- Fixed large pinned screenshots opening smaller than the selected area. A pin now starts at the capture's exact device-independent size instead of being automatically reduced to 80% of the available display.
- Fixed the built-in About Qt dialog on the dark theme: body text and links now use high-contrast colors, and the application palette is restored after the dialog closes.

### 🐛 Bug fixes

- Corrected the initial pinned-image size without changing its aspect ratio or later wheel zoom behavior.
- Restored readable Qt dialog links while keeping the rest of the application theme unchanged.
- Added regression coverage for the About Qt palette, distinct primary action icons, and exact initial pin size for a screen-sized capture.

### ✅ Verification

- CTest: 13 tests, 0 failures; 11 passed and 2 skipped as designed.
- `CaptureExclusionTests` and `NativeScreenRecorderTests` require a real desktop compositor or recording environment and were not counted as passed.

> [!WARNING]
> The final toolbar appearance, mixed-DPI pin sizing, capture exclusion, and real MP4 recording have not yet received manual desktop verification for this build.

## 简体中文

### ✨ 主要更新

- 截图工具栏 11 个图标全部替换为风格统一的独立绘制图标。贴图、复制、保存、识字、录屏、标注、历史操作和取消仍为纯图标按钮，保留 Tooltip 和无障碍名称。
- 修复大选区贴图初始显示比选区小的问题。贴图现在严格使用截图的逻辑显示尺寸，不再自动缩小到屏幕可用区域的 80%。
- 修复深色主题下 Qt 自带“关于 Qt”窗口：正文和链接统一使用高对比度颜色，窗口关闭后恢复应用原调色板。

### 🐛 问题修复

- 修正贴图初始尺寸，同时保持原始宽高比和后续滚轮缩放行为不变。
- 恢复 Qt 关于窗口链接的可读性，不影响应用其他窗口的主题。
- 新增 Qt 关于窗口配色、主要操作图标差异和屏幕级大图贴图初始尺寸回归测试。

### ✅ 验证结果

- CTest 共 13 项，零失败：11 项执行通过，2 项按设计跳过。
- `CaptureExclusionTests` 与 `NativeScreenRecorderTests` 需要真实桌面组合器或录屏环境，未计为通过。

> [!WARNING]
> 本构建尚未完成人工桌面验收：实际工具栏视觉效果、混合 DPI 贴图尺寸、捕获排除和真实 MP4 录屏仍标记为未验证。

## 📜 Open-source compliance / 开源合规

- SnipNexs: GPL-3.0-or-later.
- Qt 6.11.2: LGPL-3.0-only, dynamically linked and replaceable.
- Qt corresponding source: [`qtbase-everywhere-src-6.11.2.tar.xz`](https://github.com/YDLuo-1/SnipNexs/releases/download/v0.7.0/qtbase-everywhere-src-6.11.2.tar.xz). The same unmodified Qt build is shared by v0.7.0 and v0.7.1.
- License texts, third-party notices, the Qt SBOM, and Qt DLL replacement instructions are included in the application ZIP.
- SnipNexs：GPL-3.0-or-later。
- Qt 6.11.2：LGPL-3.0-only，采用动态链接且允许替换 Qt DLL。
- Qt 对应源码：[`qtbase-everywhere-src-6.11.2.tar.xz`](https://github.com/YDLuo-1/SnipNexs/releases/download/v0.7.0/qtbase-everywhere-src-6.11.2.tar.xz)。v0.7.0 与 v0.7.1 使用同一份未修改 Qt 构建。
- 应用 ZIP 已包含许可文本、第三方声明、Qt SBOM 和 Qt DLL 替换说明。

**Full Changelog / 完整变更：** [`v0.7.0...v0.7.1`](https://github.com/YDLuo-1/SnipNexs/compare/v0.7.0...v0.7.1)
