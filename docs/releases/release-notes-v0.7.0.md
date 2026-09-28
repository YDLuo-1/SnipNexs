# SnipNexs v0.7.0

## English

- Replaced the application and tray icon with an independently designed “capture frame + N” symbol; no assets from other screenshot tools were copied.
- Improved contrast for text, buttons, and links across the dark main window, About dialog, and open-source license dialog.
- The recording control bar now starts at the bottom-right of the selected display and can be moved by dragging an empty area; the Stop button keeps its independent click behavior and arrow cursor.
- Expanded automatic capture targeting from top-level windows to a three-level fallback: native child windows, client areas, and full-window frames. Smaller child regions take priority.
- Automatic targeting still follows Win32 HWND boundaries. Internal visual elements without a native window, such as web canvases and Qt Quick items, do not yet use UI Automation targeting.
- Added automated coverage for theme contrast, application icon generation, recording-bar placement, and window-target ordering.

### Downloads

This Release provides three separate assets: the portable application ZIP, the corresponding Qt 6.11.2 source archive, and `SHA256SUMS.txt`.

## 简体中文

- 更换为独立设计的“截图取景框 + N”应用与托盘图标，不复制其他截图软件素材。
- 深色主界面、关于页和开源许可页改用高对比度文字、按钮与链接配色。
- 录屏悬浮条默认位于所选显示器右下角，拖动空白处可以移动，停止按钮保持普通指针和独立点击行为。
- 截图自动框选从仅识别顶层窗口扩展到原生子窗口、客户区和整窗三级回退；较小的子区域优先。
- 自动框选仍以 Win32 HWND 为边界；网页画布、Qt Quick 等没有独立原生窗口的内部视觉元素暂不做 UI Automation 级识别。
- 自动测试增加主题对比度、应用图标、悬浮条位置及窗口目标排序覆盖。

### 下载

本 Release 提供三个独立附件：便携应用 ZIP、Qt 6.11.2 对应源码压缩包和 `SHA256SUMS.txt`。
