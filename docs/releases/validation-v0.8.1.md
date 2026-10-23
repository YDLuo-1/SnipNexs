# SnipNexs v0.8.1 候选版验证记录

- 日期：2026-10-24
- 系统：Windows 本机，MSVC 2022 x64
- Qt / 编译器：Qt 6.11.2 MSVC 2022 x64 / MSVC 19.44
- 发布包 SHA-256：由 `package-release.ps1` 生成并随 Release 公示

## 自动验证

- 发布代码树 CTest 共 20 项零失败：2026-10-19 开发全量运行（当次 `NativeScreenRecorderTests` 因桌面会话拒绝显示器捕获跳过，exit 77）；2026-10-24 打包运行 18 项通过、2 项按设计跳过（`LocalLlamaTests`：构建环境未安装 Hy-MT2 模型；`NativeScreenRecorderTests`：同前）。跳过逐项记录，不计为通过；CI 中录屏测试单独 `until-pass:3` 重试。
- 2026-10-24 `package-release.ps1 -Version 0.8.1` 完整通过：全新配置（RC 版本资源随主程序编译）、构建、CTest、安装、部署后 exe 文件版本检查（0.8.1）、`--self-test` 与 `--self-test-runtime` 部署自检、ZIP 生成；SHA-256 `c0f650afc0d6afb594fbc3ff71cb037a38b5a2af27681ac3bedb4db559ce2b49`。成功后脚本按规则清理了旧版 `dist` 的 0.8.0 目录与 ZIP。
- `SnipNexsRuntimeSelfTest`（新增）：无 GUI 平台插件加载 Qt TLS 后端、`ctranslate2.dll` 与 `llama.dll`，通过。
- `ReleaseLayoutTests`：部署布局包含 TLS 后端插件（`plugins/tls/qschannelbackend.dll`、`qcertonlybackend.dll`）、`vcomp140.dll`、`ctranslate2.dll`/`llama.dll`/`ggml*.dll`、MSVC CRT 与许可/SBOM 文本，通过；最终 ZIP 共 38 个条目，无 `include/`、`lib/`、spm CLI 残留（安装步骤末尾的 `install(CODE)` 剥离）。
- `CaptureHistoryStoreTests`（扩充）：索引损坏/缺失时收养未登记 PNG、不删除可读历史，通过。
- `TranslationModelsTests`（扩充）：`zh-Hans`/`en-US` 等标签归一、不支持语言组合拒绝、Hy-MT2 未知源定向中文，通过。
- `AboutDialogTests`：Qt 源码链接固定指向 `v0.7.0`（且不再出现按当前版本拼接的链接）、llama.cpp 与 ggml authors 许可文本在场，通过。

## 桌面验收

- 未验证（保持“未验证”不计入通过项）：
  - 未安装 Qt、Visual Studio 或 VC++ Redistributable 的干净 Windows 10/11 x64 上：解压直启、首次“本地翻译”模型下载（验证 TLS 插件）、两个引擎各完成一次翻译（验证 `vcomp140.dll`）。
  - Explorer 中 `SnipNexs.exe` 图标显示与文件属性版本/产品信息的人工核对；嵌入清单标识非 `com.yourcompany` 的实际核验。
  - 100% / 150% / 200% 混合缩放下的系统化 DPI 标签校对；真实区域录屏长时长稳定性。
- 以上项目完成前不得改写为“通过”；结果补充在本文件。
