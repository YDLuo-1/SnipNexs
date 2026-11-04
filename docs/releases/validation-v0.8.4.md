# SnipNexs 0.8.4 高质量单引擎本地交付

日期：2026-11-04。基于 `2a555a1c163860a237167cb9b236547aaecb066f` 的开发树构建。源码修复随本次仓库推送提交；没有创建版本标签或 GitHub Release，交付包仍为本地候选包。

## 改动

- 删除快速翻译实现、OPUS-MT 模型目录和 CTranslate2 / SentencePiece 构建及运行依赖；Hy-MT2 成为唯一本地引擎。
- OCR 结果窗口取消引擎选择框，保留自动方向、明确目标语言、对照模式和取消。不保存引擎选择设置，不提供旧配置迁移逻辑。
- 保留原 Hy-MT2 模型目录和 SHA-256，已装模型兼容；离线包预装这一套模型。
- 设置、关于、英文资源、依赖声明、打包与回归检查同步更新。旧快速模型保存在 `user-data/SnipNexs-0.8.3-win64`，不迁移进新部署或 ZIP。

## 使用与校验

完整解压 `dist/SnipNexs-0.8.4-win64.zip` 后运行 `bin/SnipNexs.exe`，或直接使用当前部署目录中的 EXE；不要单独复制 EXE。

- ZIP：1,130,971,142 字节，44 个文件，比 0.8.3 小 136.48 MiB。
- ZIP SHA-256：`b27daa68880e40e8b0e7fafd5e56f07b997a7eb92d8a0dbabe314a77afe202ad`。
- EXE SHA-256：`ab9f4dfd8c31fe202538b308853125b0b92bfb8dc4e5f0b99b574d55ed8b7112`。
- 包内只有 Hy-MT2 GGUF 和 manifest；模型文件仍为固定 SHA-256 `dc5f44fcf1fa496ee7ad725982c0c8c553a4de00259b53af84c4b89fb0c06699`。
- 包内无快速引擎 DLL、SentencePiece、OPUS 模型、历史截图或测试目录；逐文件内容已由打包脚本核验。

## 验证

- Release 构建采用 `/O2 /Ob2 /DNDEBUG`。沙箱内最初在编译器探测阶段报 C1902；同一工具链在沙箱外构建成功。
- 最终完整 CTest：30 项，24 通过、6 跳过、0 失败。移除的两项 OPUS 真实推理测试不再属于当前产品。
- Hy-MT2 自动 CPU 后端、x64 基线后端、双向文本、长文本、逐行对照、输出上限、取消后会话复用检查通过。模型仍可能误译，基线后端样例仍含未译出的英文，测试通过不代表翻译质量完全正确。
- 目标语言变化、取消/关闭窗口、并发请求隔离、模型下载的校验/取消以及旧模型保留检查通过。
- 关于窗口首次测试仍要求旧库署名，更新为检查 Hy-MT2 及已移除依赖不出现后复验通过。
- 部署后的两项自检通过；再从最终 ZIP 独立解压运行库，仅保留 Windows System32 的 PATH，通过 CTest 验证版本、Qt TLS、llama 与 CPU 后端。EXE 导入表不再依赖 CTranslate2。
- `ocr-window.png` 为 offscreen UI 测试画面，使用替代推理验证界面行为；真实模型推理证据位于 `ctest-full.log`。

跳过：NativeTrayIconTests、CaptureExclusionTests、NativeScreenRecorderTests、AudioCaptureTests、MicrophoneCaptureTests、TranslationModelDownloadTests。本轮没有桌面/音频实机、干净 Windows、旧 CPU 实机或安装器实装验收。完整模型联网下载测试须显式设置 `SNIPNEXS_TEST_LLAMA_DOWNLOAD=1`，本轮使用已有校验通过的模型。

## 收尾

当前 `dist` 仅保留 0.8.4 部署目录及各版本回退 ZIP。旧历史按原哈希迁移；旧快速模型和旧 manifest 留在 `user-data/SnipNexs-0.8.3-win64` 供恢复。原始模型和现有构建目录保留用于后续开发。

本轮清理旧运行库、重复 Hy-MT2、`build/package-temp` 和冗余编译日志，共 1,241,710,117 字节（约 1.16 GiB）。收尾未发现本项目构建、测试或应用进程遗留。

证据：`artifacts/validation-v0.8.4.json`、`package-v0.8.4.json`、`source-hashes-v0.8.4.json`、`cleanup-v0.8.4.json` 和 `artifacts/validation-v0.8.4/`。

## 配置清理与源码推送补充（2026-11-04）

按用户要求一次性删除当前用户的旧设置（Windows QSettings 使用 `HKEY_CURRENT_USER\Software\SnipNexs\SnipNexs`，不是独立 INI 文件）。清理前无运行实例，实际存在一个界面语言值；未发现旧引擎选择值。代码不增加迁移、启动时重置或其他兼容逻辑。旧引擎设置测试夹具和相关说明已删除。

本轮应用源码与已验证离线包一致，仅调整测试与文档，不重复生成同一包。最后定向复验：默认启动与 OCR 结果窗口两项 CTest 均通过，结果见本地 `artifacts/validation-v0.8.4/ctest-settings-cleanup.log`。`artifacts` 下的机器日志、截图与哈希清单仅作本地证据，已加入 Git 忽略；仓库保留本报告与回归测试源码。
