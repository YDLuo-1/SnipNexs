# SnipNexs v0.7.1

## English

- Fixed the built-in About Qt dialog on the dark theme: body text and links now use high-contrast colors, and the application palette is restored after the dialog closes.
- Replaced all 11 capture-toolbar glyphs with a consistent, independently drawn icon set. Pin, Copy, Save, OCR, Record, annotation, history actions, and Cancel remain icon-only buttons with tooltips and accessible names.
- Fixed large pinned screenshots opening smaller than the selected area. A pin now starts at the capture's exact device-independent size instead of being automatically reduced to 80% of the available display.
- Added regression coverage for the About Qt palette, distinct primary action icons, and exact initial pin size for a screen-sized capture.

### Downloads

This Release provides three separate assets: the portable application ZIP, the corresponding Qt 6.11.2 source archive, and `SHA256SUMS.txt`.

## 简体中文

- 修复深色主题下 Qt 自带“关于 Qt”窗口：正文和链接统一使用高对比度颜色，窗口关闭后恢复应用原调色板。
- 截图工具栏 11 个图标全部替换为风格统一的独立绘制图标。贴图、复制、保存、识字、录屏、标注、撤销/重做和取消仍为纯图标按钮，保留 Tooltip 和无障碍名称。
- 修复大选区贴图初始显示比选区小的问题。贴图现在严格使用截图的逻辑显示尺寸，不再自动缩小到屏幕可用区域的 80%。
- 新增 Qt 关于窗口配色、主要操作图标差异和屏幕级大图贴图初始尺寸回归测试。

### 下载

本 Release 提供三个独立附件：便携应用 ZIP、Qt 6.11.2 对应源码压缩包和 `SHA256SUMS.txt`。
