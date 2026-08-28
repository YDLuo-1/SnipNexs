# 本地翻译引擎选型决策

状态：已采纳（v0.9.0 开发中）
决策日期：2026-08-28
关联文档：[架构约束](architecture.md)、[依赖与许可](dependencies.md)、[第三方声明](../THIRD_PARTY_NOTICES.md)

本文档记录"本地翻译"功能的选型过程与最终决策，供后续维护者与 AI 代理理解为什么这样设计。除非出现本文档未预见的硬阻塞，不应在不更新本文档的情况下推翻以下结论。

## 背景与问题

v0.8.0 的翻译流程是：本地 OCR 得到文字后，由用户确认，把不超过 4000 个字符放进 `translate.google.com` 的 URL 并打开浏览器。该流程有两个根本缺陷：

1. **依赖外网与特定服务商**。Google 翻译没有桌面离线形态（离线语言包仅存在于 Android/iOS 应用），桌面网页版必须联网；且 `translate.google.com` 在中国大陆网络环境下默认不可达，对以中文为默认界面语言的用户，这个按钮大概率打不开。
2. **与产品 DNA 冲突**。SnipNexs 的既有功能全部满足"开箱即用 + 本地完成"：OCR 用 Windows 自带能力，录屏、取色、历史都在本机。翻译是唯一把用户数据送出本机的一步。

用户需求明确为：把翻译改成本地（离线）完成。

## 目标与约束

选型时以下四条来自产品既有架构约束（见 [架构约束](architecture.md)），按硬约束对待：

1. **开箱即用**：除首次下载模型包外，不要求用户安装或配置任何第三方软件。
2. **轻量**：不引入 QML/WebEngine/Chromium、脚本运行时等重依赖；推理库 + 首个语言包的总体积必须可控。
3. **隐私**：翻译在用户本机完成，文字与像素不出进程；保留"浏览器翻译"作为显式的、需确认的备选动作。
4. **架构纪律**：抽象只为隔离第三方运行时而设（引擎细节不得泄漏进 app 层）；翻译在专用工作线程执行，UI 线程不承担推理；不为假想需求预建插件系统或多后端工厂。

其他客观约束：目标平台 Windows 10 2004+ x64（含无独显的老机器，因此不能依赖 GPU）；构建链为 MSVC 2022 + Ninja + CMake 3.25+；项目许可 GPL v3+。

## 候选方案与淘汰理由

| 候选 | 结论 | 理由 |
|---|---|---|
| 云 API（百度/微软/DeepL 等） | 淘汰 | 与"本地"诉求相反；需要内置网络客户端与凭据管理，违背 translate 模块边界 |
| 嵌入 Python（LibreTranslate/Argos 解释器） | 淘汰 | 嵌入 Python 运行时违背轻量原则，部署体积与崩溃面不可接受 |
| 外挂 Ollama / LM Studio（本地大模型） | 暂缓（保留后门） | 质量最好，但要求用户先安装数 GB 的外部软件，"开箱即用"变成死按钮；为多数用户用不上的后端预建 HTTP 客户端与设置 UI 不符合架构纪律第 4 条。若未来有真实需求，在 `TranslationService` 边界之后增加一个后端即可，不影响上层 |
| Bergamot / Marian（Firefox 翻译引擎） | 淘汰 | 模型更小（约 20 MB）有吸引力，但 marian 系构建链在 Ninja + MSVC 项目中的集成本身是高风险项，且独立发行版维护已收敛 |
| ONNX Runtime + 自建管线 | 淘汰 | 分词、预处理、后处理全部需要自行拼装，工程量最大，无明显收益 |
| **CTranslate2（CPU）+ 模型按需下载** | **采纳** | 见下 |

## 采纳方案：CTranslate2 CPU 推理 + 模型包按需下载

核心思路：把 OCR 的成功模式搬到翻译上——"引擎内置、能力按需获取"。首次使用时下载一个语言包（CTranslate2 格式的模型 + 词表，约几十到一百余 MB），之后永久离线、亚秒级出结果。

选择 CTranslate2 的具体理由：

1. **纯 C++、MIT 许可**，CMake 原生，Windows/MSVC CPU-only 构建有社区实证（`WITH_CUDA=OFF`、不依赖 MKL/DNNL 亦可工作）。
2. **int8 量化模型在 CPU 上推理很快**，OCR 级别的短文本（OCR 结果通常远低于 4000 字符）在现代 CPU 上为亚秒级，无独显的老机器也可接受。
3. **模型生态现成**：Helsinki-NLP 的 OPUS-MT 模型（Marian 格式）与 Argos Translate 发布的模型包都是 CTranslate2 兼容格式，zh⇄en 无需自行训练。
4. **体积可控**：推理库约 10 MB 量级；模型按语言对下载，不随安装包膨胀。
5. **分词管线可控**：CTranslate2 的 C++ 层只接受已分词的 token 序列，模型目录词表仅支持 `.json`/`.txt`（已在 v4.8.1 源码 `src/models/model_reader.cc` 核实）；分词由 SentencePiece（Apache-2.0，纯 C++）承担，读取模型包内随附的 `.spm` 词表模型。CTranslate2 的 Python 层虽有内置 SPM，但 C++ 层没有——因此 SentencePiece 是必需依赖而非可选优化，此结论在集成阶段对源码核实后修正（初版文档曾误判可省略）。

### 模型来源策略

- **Argos Translate 现成模型包**：最初的首选，但 2026-08-28 实测其分发服务器（`pkg.argosopentech.com`、`download.argosopentech.com`）均不可达，索引仓库也无该文件；不作为依赖来源，仅在需要对照时人工取用。
- **采纳**：从 HuggingFace 拉取 Helsinki-NLP 官方仓库（`pytorch_model.bin` + `source.spm`/`target.spm`），用 CTranslate2 官方 Python 转换器转换为 int8 CTranslate2 格式，自建模型包。转换工具链仅在本机开发期使用（Python venv，位于 `build/tools-env`，不入库），发布产物只有转换结果。
- **模型包格式**：不打包 zip（避免为此引入解压库），而是逐文件分发——`model.bin`、`source_vocabulary.json`/`target_vocabulary.json`、`source.spm`/`target.spm`，每个文件在目录中静态登记 URL 与 SHA-256，客户端逐文件校验后落盘，manifest 最后写入作为安装完成标记。
- 模型许可证已核实：`Helsinki-NLP/opus-mt-zh-en` 为 CC-BY 4.0，`opus-mt-en-zh` 为 Apache 2.0，均与 GPL v3 兼容（署名要求在"关于"页与模型信息中体现）。**明确禁用 NLLB 系列（CC-BY-NC，禁止商用，与 GPL 分发不兼容）**；质量更高的 Tatoeba-Challenge tc-big 系列在 HuggingFace 检索未果（2026-08-28），列为后续观察项。未来引入其他模型时逐个核实许可证。

### 架构落位

- `translate/TranslationService`：公开接口完全镜像 `ocr/OcrService` —— 单工作线程、同一时刻一个任务、`translated`/`failed` 信号；CTranslate2 头文件只出现在私有实现（.cpp）中。
- 模型目录与下载完全复用既有模式：目录选择照抄 `CaptureHistoryStore`（exe 同目录可写则用 `models/translation/`，否则回退 Windows 本地应用数据目录）；下载落盘照抄 `index.json` 的原子替换，并用 SHA-256 校验完整性。下载用已链接但此前未实际使用的 Qt6::Network。
- 方向选择由 OCR 已返回的 `languageTag` 驱动；v1 仅支持 zh⇄en 一个语言对，跑通全链路后再扩展。
- UI 入口在 `OcrResultWindow` 增加"本地翻译"按钮，译文与原文同窗展示；本地翻译不弹"发送确认"（因为不出本机），与浏览器翻译的确认语义形成对照。
- opus-mt 有句长上限，OCR 文本按规则分句后逐句翻译，不引入第三方 NLP 分句库。

### 质量预期（明确写入用户文档）

OPUS-MT 属于"快速可用的粗翻"：对短句、日常文本够用，但断句、术语、长句质量明显低于大模型。这一限制写进 README 与架构文档，避免用户形成错误预期。追求质量的路线（Ollama 后端）留作后续，不在 v1 承诺。

### 顺手修正项（v1 范围外）

浏览器翻译指向 Google 在大陆不可达的问题，v1 通过"本地翻译"按钮的存在间接解决；把浏览器翻译目标做成可配置（Bing 等）作为后续独立小改进，不与本次引擎工作耦合。

## 实现验证记录（2026-08-28）

以下事实在集成阶段实测得出，是后续维护的重要约束：

1. **CTranslate2 必须显式选择 CPU GEMM 后端**。仅 `WITH_MKL=OFF` 的"纯净 CPU 构建"会在推理时报 `No SGEMM backend on CPU`。本项目启用 CT2 自带的 Ruy 子模块（`WITH_RUY=ON`，Apache-2.0，静态编入 `ctranslate2.dll`），不引入 MKL/oneDNN 外部依赖。
2. **MSVC 工具链上"自动计算类型 + 多 worker"会挂死**。`ComputeType::DEFAULT` 对 int8 权重模型走 Ruy 的 int8 路径并配合多线程时测试无限期阻塞；固定 `ComputeType::FLOAT32` + `num_threads_per_replica = 1` 后稳定（int8 权重在加载时反量化，下载体积优势保留）。实测 en-zh 模型加载约 255 ms、两句翻译约 493 ms，满足"OCR 级短文本亚秒级"预期。
3. **模型包自转换成功**：Helsinki-NLP opus-mt-en-zh / opus-mt-zh-en 经 `ct2-transformers-converter --quantization int8` 转换，模型约 79.5 MB/方向，双语方向均实测可翻译；文件 SHA-256 已静态登记于 `src/translate/TranslationModels.cpp`。
4. **分词边界**：SentencePiece 静态链接并通过序列化 proto 加载 `.spm`（规避 Windows 非 ASCII 路径的 `std::ifstream` 问题）；CTranslate2 词表只认 `.json`/`.txt`（`model_reader.cc`），转换产物直接兼容。
5. **测试**：`TranslationTextSplitterTests`（分句不变式）+ `LocalTranslationTests`（真实推理，模型缺失时 exit 77 跳过）；CTest 全套 16/16 通过。

## 决策的边界条件（何时应重新审视）

- 若 CTranslate2 在 MSVC 上出现无法修复的构建/运行回归，转评 Bergamot 或 ONNX Runtime，并更新本文档。
- 若用户实际反馈"要求装 Ollama 可接受且质量需求强烈"，再启用 Ollama 后端（预期为一个独立的 HTTP 后端实现，不改上层）。
- 若模型包的分发成本（GitHub Release 在部分网络的可达性）成为实际障碍，增设镜像配置项，不改变包格式与校验机制。
