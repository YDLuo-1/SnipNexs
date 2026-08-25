# SnipNexs v0.8.0 候选版验证记录

- 日期：2026-08-25
- 系统：Windows 本机，MSVC 2022 x64
- Qt / 编译器：Qt 6.11.2 MSVC 2022 x64 / MSVC 19.44
- 显示缩放：未记录；本轮没有完成真实混合 DPI 桌面操作
- 发布包 SHA-256：尚未生成；正式发布包未创建

## 自动验证

- Release 配置构建成功。
- CTest 共 14 项，零失败：12 项执行通过，`CaptureExclusionTests` 与 `NativeScreenRecorderTests` 因需要真实桌面组合器或录屏环境按设计跳过。
- `CaptureOverlayTests` 通过：13 个工具栏按钮均为纯图标并具有 Tooltip/无障碍名称；文字与识字图标不同；选区浮层显示 DPR 映射后的物理像素尺寸。
- `CaptureOverlayTests` 通过：文字输入提交后合成到原图并可撤销；取色器可复制 RGB 与 HEX；历史图仍保持原始物理分辨率与自身 DPR。
- `AnnotationDocumentTests` 通过：文字标注进入撤销文档并能完成离屏绘制。
- `PinWindowTests` 通过：高 DPI 四象限完整绘制、原始图片/DPR 导出、右键菜单、工具条显示/隐藏和左键双击关闭。
- `CaptureHistoryStoreTests` 通过：重启加载、DPR 恢复、损坏文件跳过、20 条淘汰和约 64 MiB 容量淘汰。
- 已由 CTest 生成并检查 `build/release/capture-toolbar-qa.png` 与取色面板渲染图；工具栏为浅色紧凑布局，图标使用独立绘制线条。
- 已安装到独立 QA 目录并成功启动，未出现 Qt DLL 或 MSVC 运行库缺失弹窗。

## 桌面验收

- 阻塞：Windows 桌面控制连续返回 `GetCursorPos failed: 拒绝访问 (0x80070005)`，未继续注入鼠标键盘。
- 未验证：真实按 `F1` 后的图标视觉、Tooltip、文字输入焦点与 `Esc` / `Enter` 手感。
- 未验证：真实屏幕取色值、点击复制、`C` 与 `Shift` 快捷键手感。
- 未验证：100% / 150% / 200% 混合缩放下选框物理像素标签。
- 未验证：捕获排除和真实区域录屏。

没有桌面证据的项目保持“未验证”，不计入通过项。正式打标签和创建 GitHub Release 前需补完本节。
