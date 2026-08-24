# 架构约束

SnipNexs 采用单进程、模块化单体架构。现阶段不使用公共 DLL 插件系统：它会增加 ABI、部署、调试和安全成本，而截图工具的内置功能尚不需要跨版本二进制扩展。

## 模块边界

依赖方向统一由上层功能指向下层能力：

```text
app
 ├─ capture ── platform/windows
 ├─ editor
 ├─ pin
 ├─ ocr ────── Windows.Media.Ocr
 ├─ translate ─ browser
 └─ recorder ─ Windows.Graphics.Capture + D3D11 + Windows.Media
```

- `app`：生命周期、单实例、托盘、快捷键、语言切换和窗口协调。
- `capture`：屏幕/窗口/区域采集及捕获会话状态。
- `editor`：选区、绘制命令、撤销重做与导出。
- `pin`：贴图窗口及其交互，不复制编辑器内部状态。
- `ocr`：OCR 请求、后台调度和系统 OCR 适配。
- `translate`：只生成显式浏览器翻译 URL；当前没有内置网络客户端或凭据。
- `recorder`：帧采集、队列、编码与封装。
- `platform/windows`：Win32、D3D11、Windows Graphics Capture 等平台代码。

目录在功能落地时才创建；不会为路线图中的假想实现预建空类。

## 当前实现（0.6.0）

- `app/MainWindow` 负责主窗口、托盘、关于入口和状态展示，通过 `captureRequested` 信号请求截图；有托盘时启动不显示主界面，托盘不可用时才回退显示。
- `app/AboutDialog` 独立显示版本、项目许可、Qt 许可与第三方声明；文本作为 Qt 资源内嵌，离线可读，不复制其他截图软件的界面或素材。
- 界面以中文源码文案为默认语言，英文由内嵌 Qt `.qm` 资源提供；`QSettings` 只保存 `zh_CN` 或 `en`，不增加自定义国际化框架。
- `capture/CaptureController` 协调窗口隐藏、显示器采集、剪贴板和文件保存；捕获期间 `MainWindow` 会拒绝托盘或重复启动触发的重新显示。Windows 10 2004 及以上还会在捕获前请求 `WDA_EXCLUDEFROMCAPTURE`，避免 DWM 的旧窗口帧进入截图。
- `capture/CaptureOverlay` 负责选区状态、标注交互与轻量绘制；原始 `QPixmap` 保持隐式共享，用户确认后才转换和裁剪为 `QImage`。
- 只把成功复制、保存或贴出的截图加入会话内历史；`,` / `.` 向前或向后浏览。历史同时受 20 条和约 64 MiB 上限约束，不写磁盘。历史原图与缩小预览矩形分开保存，导出始终保留原始物理分辨率和图片自己的 DPR；历史模式固定整图选择，标注坐标在导出时映射回原图。
- `editor/AnnotationDocument` 保存画笔、矩形和箭头数据及撤销游标；复制、保存或贴图时才合成标注。
- `pin/PinWindow` 是仅持有一份隐式共享 `QPixmap` 的置顶窗口，支持移动和以鼠标内容为锚点的累积有界缩放；阴影由窗口直接绘制，不在每次缩放时创建图形场景或模糊效果。
- `ocr/OcrService` 复用一个工作线程，避免 OCR 阻塞 UI；同一时刻只接受一个识别任务。
- `ocr/WindowsOcr` 把 `QImage` 行数据直接复制到 `SoftwareBitmap`，不经过 PNG/BMP 编解码，也不捆绑 OCR 模型。
- `translate/BrowserTranslation` 只构造文本翻译 URL；结果窗口在打开浏览器前明确提示传输边界。
- `recorder/RecorderController` 管理路径、临时文件、悬浮停止条和录制生命周期；完成的 MP4 通过同目录原子替换落到用户目标路径。
- `recorder/ScreenRecorderService` 只维护一个后台录制线程；停止请求是原子标志，不阻塞 UI。
- `recorder/NativeScreenRecorder` 使用 Windows Graphics Capture 获取 GPU 表面，D3D11 裁剪后直接交给 Windows 媒体管线，不做 CPU 像素回读。
- 帧池容量为 2，应用只保存最新帧；慢编码时覆盖旧帧，不让内存随录制时长增长。
- `platform/windows/GlobalHotkey` 是唯一的 Win32 快捷键边界，使用 `RegisterHotKey`；默认注册 F1，失败时回退到 `Ctrl+Shift+A`，未引入第三方热键库或全局键盘钩子。
- 当前选区限定在鼠标所在显示器，已处理显示缩放比例；跨显示器单次选区尚未实现。

## 扩展规则

1. 模块通过窄的数据结构或 Qt 信号连接，不直接访问其他模块的窗口控件。
2. 只有当一个边界已存在两个实现，或必须隔离第三方运行时时，才引入抽象接口。
3. 内置功能保持静态编译；脚本/二进制插件系统必须有真实用例和稳定 API 后再评估。
4. UI 线程只处理交互与轻量绘制；OCR 和编码放入有界工作队列，未来的内置网络翻译也必须遵守此规则。
5. 图像默认使用隐式共享的 `QImage`，写入前才分离，避免无意义深拷贝。

## 性能与验证

- 截图到首帧显示目标：常用桌面环境 P95 小于 100 ms。
- 编辑交互目标：60 Hz 显示器下不出现可感知掉帧。
- 录屏队列已有固定上限；结束结果记录捕获帧、向媒体管线提交的帧、总耗时和文件大小。
- 每个阶段至少包含可自动执行的构建/自检；性能结论只能来自对应完整链路的测量。
