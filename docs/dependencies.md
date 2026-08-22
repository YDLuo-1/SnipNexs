# 依赖与许可

| 依赖 | 当前用途 | 链接方式 | 许可证注意事项 |
|---|---|---|---|
| Qt 6.8.3 Core/Gui/Widgets/Network | UI、事件循环、单实例通信、屏幕采集、标注与贴图 | 动态链接 | LGPL-3.0-only；随包发布许可文本并允许替换 DLL |
| Windows SDK / User32 | Windows 10 API、全局快捷键 | 系统组件 | 随 MSVC/Windows SDK 条款 |
| Windows.Media.Ocr / C++/WinRT | 本地 OCR；语言模型由 Windows 语言包提供 | 系统 API、`windowsapp.lib` | 不随 SnipNexs 分发 OCR 模型或第三方 OCR DLL |

当前没有复制 eSearch、Flameshot、Ksnip、ShareX、OBS 或 Snipaste 的源代码，也没有引入 Apache-2.0 OCR 组件。`translate.google.com` 只是用户确认后由默认浏览器打开的外部服务，不是链接或捆绑依赖；发送内容上限为 4000 个字符，不发送图像。

[微软当前文档](https://learn.microsoft.com/en-us/uwp/api/windows.media.ocr)将 `Windows.Media.Ocr` 的桌面正式支持限定为具有包身份的应用。当前便携构建已在无包身份进程中通过实际识别测试，但 Windows 10 22H2 便携环境仍需单独验证；运行时失败会显示错误，不会回退到在线 OCR。
