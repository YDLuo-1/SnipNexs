# 同类项目源码核对（2026-08-22）

本文件记录实际查看的源码快照、依赖和可复用结论。这里只学习结构，不复制实现。

## 已确认事实

| 项目与快照 | 技术与关键依赖 | OCR / 翻译 / 录屏实现 | 对 SnipNexs 的结论 |
|---|---|---|---|
| [eSearch 15.4.2](https://github.com/xushengfeng/eSearch/tree/186c4f5d12581ff2161efa645efe065c66f853c3) | Electron 43.4、TypeScript、`node-screenshots 0.2.8`、`onnxruntime-node 1.27.0` | `esearch-ocr 8.5.2` 加 PP-OCRv6 ONNX 模型；`xtranslator 1.5.9`；Chromium `MediaRecorder` 先累积 Blob，再调用 FFmpeg 6.1 裁剪/转码 | 功能覆盖最接近，但 Electron、Node、ONNX Runtime、模型和 FFmpeg 同包不符合当前轻量目标；不能采用其 Blob 累积录屏路径 |
| [Flameshot](https://github.com/flameshot-org/flameshot/tree/71bb194394ab49dee60cad72a3904d3225154ef5) | C++/Qt 6、CMake、QHotkey、KDSingleApplication；OpenSSL 可选 | 当前源码聚焦截图、标注、上传，没有内置 OCR 或录屏 | 证明纯 Qt Widgets 截图工具可长期维护；其成熟工具工厂规模对当前 SnipNexs 过重 |
| [ksnip 1.11.0](https://github.com/ksnip/ksnip/tree/e80075590d72da652da787fabe2208307c32fc18) | C++17、Qt 5/6、kImageAnnotator、kColorPicker、Qt 动态插件 | [OCR 插件](https://github.com/ksnip/ksnip-plugin-ocr/tree/b14e001bd6ea96e582ebbbe3759e7c8e52be8ce7) 链接 Tesseract/Leptonica；Windows 从插件旁读取 `tessdata` | OCR 边界值得保留；公共 Qt 插件要求匹配 Qt 版本和构建类型，当前不值得承担 ABI/部署成本 |
| [ShareX 21](https://github.com/ShareX/ShareX/tree/064144546895ca0b3e0348649120e360e484c843) | .NET 10、WinForms/Avalonia 模块、System.Drawing、SkiaSharp、DirectML 等 | OCR 用 `Windows.Media.Ocr` 并放入后台任务；录屏用外部 FFmpeg，支持 `gdigrab`、`ddagrab`、dshow 及 NVENC/AMF/QSV | 系统 OCR 是零模型包的最轻路线；录屏应把捕获、编码器选择和进程管理分开，但不复制其庞大选项面 |

ksnip OCR 插件还有一个许可证元数据不一致点：仓库 `LICENSE.txt` 是 GPLv2 文本，而 `OcrWrapper.cpp` 文件头声明 LGPLv2-or-later。因此 SnipNexs 不复制该插件源码或许可证结论。

## 当前采用方案

- OCR：Windows `Windows.Media.Ocr`，单后台线程，`QImage` 直接复制到 `SoftwareBitmap`；不加入 Tesseract、PaddleOCR、ONNX Runtime、OpenCV 或模型文件。
- 翻译：结果窗口在用户确认后用默认浏览器打开文字翻译 URL；应用本身不保存密钥、不发 HTTP、不上传图片。
- 扩展：只有出现第二个真实 OCR/翻译实现时才抽象公共 provider 接口；现阶段保持模块化单体。
- 录屏：后续使用有界帧队列和 Windows GPU 捕获/硬件编码路径；不会采用无限 Blob 或全帧 CPU 往返。

## 尚未验证

- 微软当前文档要求桌面应用具有包身份才能获得 `Windows.Media.Ocr` 的正式支持。无包身份便携程序已在当前 Windows 环境完成真实识别，但 Windows 10 22H2 尚未实机验证。
- 当前 OCR 测试图为程序生成的数字图，只证明图像转换、系统引擎和文本返回链路；不代表中文、复杂排版或多屏截图的准确率。
- Snipaste 不是开源项目，无法进行同等级源码核对。
