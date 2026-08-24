# v0.6.0 发布清单

## 自动门禁

1. `CMakeLists.txt` 版本为 `0.6.0`，HEAD 精确带有 `v0.6.0` 标签，工作树干净。
2. 运行 `scripts/package-release.ps1 -Version 0.6.0`。脚本必须完成配置、构建、CTest、安装、部署自检、ZIP 和校验文件生成。
3. CTest 必须零失败；输出中被跳过的项目必须逐项记录，不能把“执行通过”和“跳过”合并成 11/11。
4. ZIP 必须包含 Qt Core/Gui/Widgets/Network、Windows 平台插件、MSVC CRT、GPL/LGPL/MIT 文本、Qt 人类可读声明和 `qtbase-6.11.2.spdx`。
5. `qtbase-everywhere-src-6.11.2.tar.xz` 的 SHA-256 必须是 `5b2e00eccaf5a4d8c14134ffa0ea8dfd0a35ae1ffc7f8d87fa4305a1ed23cf22`。

## 桌面验收

- 全新解压到不含 Qt/VC DLL 的目录后可直接启动，无系统错误框。
- 有托盘时启动不显示主界面；双击托盘、菜单“打开 SnipNexs”和再次运行程序均可打开主界面。
- 截图开始前主界面消失；截图中托盘、关于入口和再次运行均不能让主界面覆盖截图；取消后可正常恢复。
- 选区可移动和用八个控制点缩放；贴图可拖动，连续滚轮缩放有效，鼠标所指内容位置稳定。
- `,` / `.` 可切换会话历史；在 100%/150%/200% 显示缩放环境抽查预览、复制、保存、贴图和标注坐标。
- 主界面和托盘都能打开“关于 SnipNexs”；中英文版本、GPL/LGPL、Qt 源码链接和“关于 Qt”均正确。
- 区域录屏可开始、停止并生成 MP4；捕获排除可在真实桌面组合器下验证。
- 从托盘退出后没有残留 `SnipNexs.exe` 进程。

桌面验收应写入 `docs/validation-v0.6.0.md`，记录日期、系统、显示缩放、发布包哈希、逐项结果和未验证项。没有证据的项目必须写“未验证”，不能写“通过”。

## GitHub 发布

认证有效后：

```powershell
git push origin main
git push origin v0.6.0
gh release create v0.6.0 `
  dist\SnipNexs-0.6.0-win64.zip `
  dist\qtbase-everywhere-src-6.11.2.tar.xz `
  dist\SHA256SUMS.txt `
  --title "SnipNexs v0.6.0" --notes-file docs\release-notes-v0.6.0.md
```

若 `gh auth status` 或 Git 推送认证失败，停止上传并如实报告“本地完成、尚未发布”。
