# SnipNexs v0.8.0

> [!IMPORTANT]
> Windows 10/11 x64 portable build. Extract the ZIP before running `bin/SnipNexs.exe`; do not run it inside the archive.
>
> Windows 10/11 x64 便携版。请先完整解压 ZIP，再运行 `bin/SnipNexs.exe`，不要直接在压缩包内启动。

## 📦 Download / 下载

| File / 文件 | Purpose / 用途 |
| --- | --- |
| `SnipNexs-0.8.0-win64.zip` | Portable application / 免安装便携应用 |
| `translation-models-v1` | Default offline translation packages (downloaded in-app on first use) / 默认离线翻译语言包（应用内按需下载） |
| `translation-models-hymt2-v1` | Optional high-quality engine model (~1.1 GB, downloaded in-app) / 可选高质量引擎模型（应用内按需下载） |

> [!TIP]
> Most users only need `SnipNexs-0.8.0-win64.zip`; the translation models download automatically on first use of the feature.
>
> 普通用户只需要下载 `SnipNexs-0.8.0-win64.zip`；翻译模型会在首次使用该功能时于应用内自动下载。

## English

### ✨ Highlights

- **Fully offline local translation with two engines.** The OCR result window can translate recognized text without any network: the default engine (CTranslate2 + OPUS-MT int8, ~80 MB per direction, sub-second) produces quick draft-grade output, and a switchable high-quality engine (Tencent Hy-MT2-1.8B via llama.cpp, ~1.1 GB, Apache-2.0) trades download size and seconds-per-segment for near-LLM quality. Packages are SHA-256 verified against a pinned catalog, installed with atomic manifests, work behind a configurable mirror, and keep running fully offline after download.
- **"识字" (OCR) and 本地翻译 are joined by an actionable failure path**: missing Windows OCR language packs now explain exactly where to enable text recognition in Windows Settings.
- Reworked the capture toolbar as a compact light surface with pixel-precise Material Symbols Rounded glyphs (solid tools + outlined geometry, the Snipaste-like mixed language), a drop shadow, and denser buttons. Icons render at the exact logical size × device pixel ratio, staying crisp on 100%–200% displays.
- Added inline single-line text annotation, a fully local color picker (9 × 9 magnified grid, RGB/HEX, `C`/`Shift`/click shortcuts), physical pixel dimensions next to the selection, and pinned images with cursor-anchored zoom that now shows the live zoom percentage.
- Added a hidden-by-default compact pin toolbar, persistent local capture history (`history` beside the exe, 20 PNGs / ~64 MiB, restored after restart), and single-instance tray-first startup with `F1`/`Ctrl+Shift+A`.
- Startup path no longer loads the translation engines: `ctranslate2.dll` and `llama.dll` are delay-loaded on first use.

### ✅ Verification

- CTest: 19 tests, 0 failures across two consecutive full runs; engine-dependent tests skip by design when models/GPU are absent.
- Real inference smoke tests: OPUS-MT loads in ~255 ms and translates in ~500 ms; Hy-MT2-1.8B loads in ~0.9 s and translates two segments in ~1 s.
- All 10 published translation model assets verified `state=uploaded` with exact sizes; download URLs return HTTP 200 and end-to-end SHA-256 matches the in-app catalog.
- Deployment layout verified: 44 MB, engines delay-loaded (`dumpbin` shows them in the delay-load section), no stray third-party CLI tools or dev libraries.
- Icon pipeline (`tools/icongen/`) regenerates `ToolbarIcons.cpp` byte-identically from vendored Apache-2.0 SVG sources.

> [!WARNING]
> Desktop interaction (real `F1` capture, zoom-indicator feel, mixed-DPI labels) was exercised informally by the developer but remains outside the automated suite; report issues via GitHub Issues.
>
> 桌面交互（真实 `F1` 截图、缩放比手感、混合 DPI 标签）由开发者非正式试用过，但不在自动化范围内；问题请通过 GitHub Issues 反馈。

## 简体中文

### ✨ 主要更新

- **双引擎完全离线本地翻译。** OCR 结果窗口可对识别文字直接离线翻译：默认引擎（CTranslate2 + OPUS-MT int8，约 80 MB/方向，亚秒级）输出快速粗翻；可切换高质量引擎（腾讯 Hy-MT2-1.8B，经 llama.cpp CPU 推理，约 1.1 GB，Apache-2.0），用下载体积和每段数秒换取接近大模型的质量。语言包按固定目录的 SHA-256 校验、原子 manifest 安装、支持镜像配置，下载一次后永久离线。
- **“识字”失败路径可行动**：缺少 Windows OCR 语言包时，提示会明确给出系统设置中的开启路径。
- 截图工具栏重做：紧凑浅色面板 + 像素级精确的 Material Symbols Rounded 字形（实心工具 + 描边几何的混合语言）、柔和投影、更紧凑的按钮；图标按逻辑尺寸 × 设备像素比直接渲染，100%–200% 缩放下都清晰。
- 新增单行文字标注、完全本地的取色器（9 × 9 放大格、RGB/HEX、`C`/`Shift`/单击复制）、选框旁的物理像素尺寸；贴图滚轮缩放现在实时显示缩放百分比。
- 新增默认隐藏的紧凑贴图工具条、持久化截图历史（程序目录 `history`，20 张/约 64 MiB，重启恢复）、托盘化单实例启动与 `F1`/`Ctrl+Shift+A`。
- 启动路径不再加载翻译引擎：`ctranslate2.dll` 与 `llama.dll` 首次使用时才延迟加载。

### ✅ 验证结果

- CTest 19 项，连续两轮零失败；依赖模型/GPU 的测试按设计跳过。
- 真实推理冒烟：OPUS-MT 加载约 255 ms、翻译约 500 ms；Hy-MT2-1.8B 加载约 0.9 s、两段翻译约 1 s。
- 已发布的 10 个翻译模型资产逐一核验 `state=uploaded` 与精确大小；下载 URL 返回 200，端到端 SHA-256 与应用内目录一致。
- 部署布局核验：44 MB，引擎处于延迟加载段（`dumpbin` 确认），无第三方 CLI 工具或开发库残留。
- 图标管线（`tools/icongen/`）可从入库的 Apache-2.0 SVG 源逐字节再生成 `ToolbarIcons.cpp`。

> [!WARNING]
> 桌面交互（真实 `F1` 截图、缩放比手感、混合 DPI 标签）由开发者非正式试用过，但不在自动化范围内；问题请通过 GitHub Issues 反馈。

## 📜 Open-source compliance / 开源合规

- SnipNexs: GPL-3.0-or-later.
- Qt 6.11.2: LGPL-3.0-only, dynamically linked and replaceable.
- Local translation engines: CTranslate2 (MIT) + SentencePiece (Apache-2.0) + llama.cpp (MIT); translation models: Helsinki-NLP OPUS-MT (Apache-2.0/CC-BY 4.0) and Tencent Hy-MT2-1.8B (Apache-2.0). Toolbar glyph geometry: Material Symbols Rounded (Apache-2.0).
- Qt corresponding source: [`qtbase-everywhere-src-6.11.2.tar.xz`](https://github.com/YDLuo-1/SnipNexs/releases/download/v0.7.0/qtbase-everywhere-src-6.11.2.tar.xz). The same unmodified Qt build is shared by v0.7.0 through v0.8.0.
- License texts, third-party notices, the Qt SBOM, and Qt DLL replacement instructions are included in the application ZIP.
- SnipNexs：GPL-3.0-or-later。
- Qt 6.11.2：LGPL-3.0-only，动态链接且允许替换 Qt DLL。
- 本地翻译引擎：CTranslate2（MIT）+ SentencePiece（Apache-2.0）+ llama.cpp（MIT）；翻译模型：Helsinki-NLP OPUS-MT（Apache-2.0/CC-BY 4.0）与腾讯 Hy-MT2-1.8B（Apache-2.0）。工具栏字形：Material Symbols Rounded（Apache-2.0）。
- Qt 对应源码：[`qtbase-everywhere-src-6.11.2.tar.xz`](https://github.com/YDLuo-1/SnipNexs/releases/download/v0.7.0/qtbase-everywhere-src-6.11.2.tar.xz)。v0.7.0 至 v0.8.0 共用同一份未修改 Qt 构建。
- 应用 ZIP 包含许可文本、第三方声明、Qt SBOM 和 Qt DLL 替换说明。

**Full Changelog / 完整变更：** [`v0.7.1...v0.8.0`](https://github.com/YDLuo-1/SnipNexs/compare/v0.7.1...v0.8.0)
