# SnipNexs v0.8.1

> [!IMPORTANT]
> Windows 10/11 x64 portable build. Extract the ZIP before running `bin/SnipNexs.exe`; do not run it inside the archive.
>
> Windows 10/11 x64 便携版。请先完整解压 ZIP，再运行 `bin/SnipNexs.exe`，不要直接在压缩包内启动。

## 📦 Download / 下载

| File / 文件 | Purpose / 用途 |
| --- | --- |
| `SnipNexs-0.8.1-win64.zip` | Portable application / 免安装便携应用 |
| `translation-models-v1` | Default offline translation packages (downloaded in-app on first use) / 默认离线翻译语言包（应用内按需下载） |
| `translation-models-hymt2-v1` | Optional high-quality engine model (~1.1 GB, downloaded in-app) / 可选高质量引擎模型（应用内按需下载） |

> [!TIP]
> Most users only need `SnipNexs-0.8.1-win64.zip`; the translation models download automatically on first use of the feature.
>
> 普通用户只需要下载 `SnipNexs-0.8.1-win64.zip`；翻译模型会在首次使用该功能时于应用内自动下载。

## English

### ✨ Highlights

- **Fixed the portable package's missing runtime pieces (the reason for this release).** The v0.8.0 ZIP omitted the Qt TLS backend plugin and `vcomp140.dll`: on a clean Windows system, the first translation-model download (HTTPS) and the default CTranslate2 engine (OpenMP runtime) both failed. The package now ships `plugins/tls/qschannelbackend.dll` and `vcomp140.dll` next to the engines, and a new `--self-test-runtime` mode — run in CTest and again by the packaging script against the staged layout — verifies that the TLS backend, `ctranslate2.dll` and `llama.dll` all load from the deployed directory itself.
- **Windows app icon and version resource.** Explorer now shows the SnipNexs icon for `SnipNexs.exe`; file properties display version 0.8.1 with product information; the embedded manifest identifier is `io.github.ydluo.snipnexs` instead of a generic placeholder. The optional Inno Setup installer uses the same icon and no longer deletes user data (`history`, `models`) on uninstall.
- **Capture history survives a damaged index.** A missing or corrupt `index.json` no longer risks deleting captures: readable PNGs missing from the index are adopted into the history, and loading no longer decodes every PNG just to validate the index.
- **Local translation fixes.** Language-direction resolution now handles tags like `zh-Hans`/`en-US` and rejects unsupported pairs explicitly; mirror base URLs are joined as URLs (a path join used to collapse the `//` in `https://`); failed download attempts no longer accumulate progress state or signal connections; stale translation results arriving after a new request are ignored; the Hy-MT2 engine clears its context between segments so long texts no longer exhaust the 2048-token window; generation threads are clamped to half the logical processors (max 8).
- **Smaller fixes.** Save dialogs now apply the selected filter's suffix (`png`/`jpg`/`bmp`); a second launch racing the first at startup activates the running instance instead of tearing down its endpoint; the color picker stops re-converting the screenshot on every mouse move; recording start failures clean up their partial-file state; the About dialog's Qt source link is pinned to the shared `v0.7.0` asset and the llama.cpp (MIT) license is listed.

### ✅ Verification

- CTest on the release tree: 20 tests, 0 failures. Full development run 2026-10-19 and release packaging run 2026-10-24 (18 passed in the packaging run; `LocalLlamaTests` skipped because the build environment has no Hy-MT2 model installed, and `NativeScreenRecorderTests` skipped because the desktop session denied monitor capture — both exit 77 by design, recorded as skipped and never counted as passed).
- The release packaging run verified end to end: fresh configure and build, the full suite above, install, deployed `SnipNexs.exe` file version equals 0.8.1, `--self-test` passes, and `--self-test-runtime` loads the Qt TLS backend plus both engine DLLs from the staged `dist` layout. The install step also strips third-party dev files (`include/`, `lib/`, SentencePiece CLI tools), leaving a 38-entry package.
- `ReleaseLayoutTests` checks the ZIP staging for the TLS plugin, `vcomp140.dll`, `ctranslate2.dll`/`llama.dll`/`ggml*.dll`, the MSVC CRT, and all license/SBOM texts.

> [!WARNING]
> Not yet verified on a clean Windows 10/11 x64 VM (no Qt / Visual Studio / VC++ Redistributable preinstalled): first-run model download over TLS, both engines completing a translation, and the Explorer icon/file-properties display. These items stay marked 未验证 in `docs/releases/validation-v0.8.1.md` until that pass is done.
>
> 尚未在干净 Windows 10/11 x64 虚拟机（未预装 Qt / Visual Studio / VC++ Redistributable）上逐项验收：首次模型下载（TLS）、双引擎各完成一次翻译、Explorer 图标与文件属性显示。完成前这些项在 `docs/releases/validation-v0.8.1.md` 中保持“未验证”。

## 简体中文

### ✨ 主要更新

- **修复便携包缺失的运行时组件（本次发布的原因）。** v0.8.0 的 ZIP 缺少 Qt TLS 后端插件和 `vcomp140.dll`：在干净的 Windows 系统上，首次下载翻译模型（HTTPS）与默认 CTranslate2 引擎（OpenMP 运行库）都会失败。包内现随引擎附带 `plugins/tls/qschannelbackend.dll` 与 `vcomp140.dll`，并新增 `--self-test-runtime` 模式——CTest 与打包脚本都会对部署目录实际加载 TLS 后端、`ctranslate2.dll` 与 `llama.dll` 验证。
- **Windows 应用图标与版本资源。** Explorer 中 `SnipNexs.exe` 显示应用图标；文件属性显示 0.8.1 版本与产品信息；嵌入清单标识为 `io.github.ydluo.snipnexs`，不再是占位符。可选的 Inno Setup 安装器使用同一图标，卸载时不再删除用户数据（`history`、`models`）。
- **截图历史不再受索引损坏影响。** `index.json` 缺失或损坏时不再有删图风险：未登记但可读的 PNG 会被收养进历史；加载也不再为校验索引而逐张解码 PNG。
- **本地翻译修复。** 方向解析正确处理 `zh-Hans`/`en-US` 等标签并明确拒绝不支持的语言组合；镜像地址按 URL 拼接（原先路径拼接会折叠 `https://` 的双斜杠）；下载失败重试不再累积进度状态与信号连接；过期翻译结果被忽略；Hy-MT2 引擎每段翻译前清空上下文，长文本不再耗尽 2048 token 窗口；生成线程数收敛为逻辑处理器一半（上限 8）。
- **其他修复。** 保存对话框按所选过滤器补全后缀（`png`/`jpg`/`bmp`）；启动竞态时二次启动改为唤醒已运行实例；取色器不再每次鼠标移动都重新转换截图；录屏启动失败清理残缺文件状态；关于对话框的 Qt 源码链接固定指向 `v0.7.0` 的共用副本，并补列 llama.cpp（MIT）许可。

### ✅ 验证结果

- 发布代码树 CTest 20 项零失败：2026-10-19 开发全量运行与 2026-10-24 打包运行（打包运行 18 项通过；`LocalLlamaTests` 因构建环境未安装 Hy-MT2 模型跳过、`NativeScreenRecorderTests` 因桌面会话拒绝显示器捕获跳过——均按设计 exit 77，记录为跳过、绝不计为通过）。
- 打包运行端到端验证：全新配置与构建、上述全量测试、安装、部署后的 `SnipNexs.exe` 文件版本为 0.8.1、`--self-test` 通过、`--self-test-runtime` 在 `dist` 部署目录中实际加载 Qt TLS 后端与两个引擎 DLL。安装步骤同时剥离第三方开发文件（`include/`、`lib/`、SentencePiece 命令行工具），最终包共 38 个条目。
- `ReleaseLayoutTests` 校验包内 TLS 插件、`vcomp140.dll`、`ctranslate2.dll`/`llama.dll`/`ggml*.dll`、MSVC CRT 与全部许可/SBOM 文本。

> [!WARNING]
> 尚未在干净 Windows 10/11 x64 虚拟机（未预装 Qt / Visual Studio / VC++ Redistributable）上逐项验收：首次模型下载（TLS）、双引擎各完成一次翻译、Explorer 图标与文件属性显示。完成前这些项在 `docs/releases/validation-v0.8.1.md` 中保持“未验证”。

## 📜 Open-source compliance / 开源合规

- SnipNexs: GPL-3.0-or-later.
- Qt 6.11.2: LGPL-3.0-only, dynamically linked and replaceable.
- Local translation engines: CTranslate2 (MIT) + SentencePiece (Apache-2.0) + llama.cpp (MIT); translation models: Helsinki-NLP OPUS-MT (Apache-2.0/CC-BY 4.0) and Tencent Hy-MT2-1.8B (Apache-2.0). Toolbar glyph geometry: Material Symbols Rounded (Apache-2.0). App icon geometry matches the in-app icon and is generated by `tools/icongen/`.
- Qt corresponding source: [`qtbase-everywhere-src-6.11.2.tar.xz`](https://github.com/YDLuo-1/SnipNexs/releases/download/v0.7.0/qtbase-everywhere-src-6.11.2.tar.xz). The same unmodified Qt build is shared by v0.7.0 through v0.8.1.
- License texts, third-party notices, the Qt SBOM, and Qt DLL replacement instructions are included in the application ZIP.
- SnipNexs：GPL-3.0-or-later。
- Qt 6.11.2：LGPL-3.0-only，动态链接且允许替换 Qt DLL。
- 本地翻译引擎：CTranslate2（MIT）+ SentencePiece（Apache-2.0）+ llama.cpp（MIT）；翻译模型：Helsinki-NLP OPUS-MT（Apache-2.0/CC-BY 4.0）与腾讯 Hy-MT2-1.8B（Apache-2.0）。工具栏字形：Material Symbols Rounded（Apache-2.0）。应用图标几何与应用内图标一致，由 `tools/icongen/` 生成。
- Qt 对应源码：[`qtbase-everywhere-src-6.11.2.tar.xz`](https://github.com/YDLuo-1/SnipNexs/releases/download/v0.7.0/qtbase-everywhere-src-6.11.2.tar.xz)。v0.7.0 至 v0.8.1 共用同一份未修改 Qt 构建。
- 应用 ZIP 包含许可文本、第三方声明、Qt SBOM 和 Qt DLL 替换说明。

**Full Changelog / 完整变更：** [`v0.8.0...v0.8.1`](https://github.com/YDLuo-1/SnipNexs/compare/v0.8.0...v0.8.1)
