# SnipNexs v0.8.0 候选版验证记录

- 日期：2026-10-03（更新至最终发布内容）
- 系统：Windows 本机，MSVC 2022 x64
- Qt / 编译器：Qt 6.11.2 MSVC 2022 x64 / MSVC 19.44
- 发布包 SHA-256：由 `package-release.ps1` 生成并随 Release 公示

## 自动验证（2026-10-03）

- Release 配置构建成功；打包脚本前置条件（干净工作树、HEAD 带 v0.8.0 标签、CMake 版本匹配）全部满足。
- CTest 共 19 项，连续两轮零失败；依赖模型/GPU 的测试（本地翻译真实推理、Windows OCR、录屏、捕获排除）在环境不具备时按设计跳过（exit 77）。
- 引擎真实推理冒烟：OPUS-MT int8 加载约 255 ms、两句翻译约 493 ms；Hy-MT2-1.8B Q4 加载约 0.9 s、两句翻译约 1 s，输出为正确中文/英文。
- `PinWindowTests`：高 DPI 四象限、原始图片/DPR 导出、右键菜单、工具条、双击关闭、滚轮缩放锚点与缩放比标签全部通过；30 连跑稳定。
- `ToolbarIconsTests`：13 个字形着墨量与两两差异回归通过；`AboutDialogTests`：5 个许可标签（含 Material Symbols 与翻译引擎）通过。
- `TranslationModelsTests`：引擎→语言包目录解析逻辑（含方向回退守卫）通过。
- 模型分发：`translation-models-v1`（10 资产）与 `translation-models-hymt2-v1`（1 资产）全部 `state=uploaded` 且大小精确匹配；下载 URL 实测 200；端到端 SHA-256 与 `TranslationModels.cpp` 目录一致。
- 部署布局（`ReleaseLayoutTests` + dumpbin 抽查）：44 MB；`ctranslate2.dll`/`llama.dll` 处于 delay load 段；无 spm CLI 工具、无第三方 include/lib 残留。
- 启动延迟加载：`SnipNexsSelfTest` 通过，进程启动不再静态导入翻译引擎 DLL。
- 图标管线：`tools/icongen/gen_material.py` 从入库 SVG 源再生成 `ToolbarIcons.cpp`,与提交内容一致。

## 桌面验收

- 开发者日常使用中非正式试用：F1 截图、标注、取色、识字→本地翻译（双引擎）、贴图缩放与缩放比标签均实际操作过，未发现阻塞问题。
- 未验证（保持“未验证”不计入通过项）：100% / 150% / 200% 混合缩放下的系统化 DPI 标签校对;真实区域录屏的长时长稳定性;Hy-MT2 大模型在低端 CPU（如 2 核老机器）上的可用速度。
