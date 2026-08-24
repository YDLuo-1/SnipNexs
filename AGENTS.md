# 项目代理规则

## Windows 与 Qt 测试

- Qt 测试必须通过 CTest 运行，例如：
  `ctest --test-dir build/release --output-on-failure`。
- 禁止直接启动 `build/**/**Tests.exe`。这些测试程序依赖 CTest 注入 Qt DLL 的 `PATH`；直接运行会因找不到 `Qt6Widgets.dll` 等依赖而弹出 Windows 系统错误框。
- 非交互测试必须使用 CMake/CTest 中配置的 `QT_QPA_PLATFORM=offscreen`，不得抢占用户桌面或显示测试窗口。
- 只有用户明确要求桌面交互验收时，才启动带完整部署依赖的 `dist` 发布程序；启动前先确认不会与已有 SnipNexs 实例冲突，结束后正常退出测试实例。
- 如果必须绕过 CTest 单独诊断测试程序，先显式配置 Qt `bin` 到当前进程的 `PATH`，并确保测试窗口不会显示到用户桌面。

## dist 目录管理

- 新的 `dist` 部署目录生成并验证成功后，只保留当前最新的有效部署目录，及时删除旧的 QA/临时部署目录，避免重复占用空间和误测旧版本。
- 删除前必须解析并核对绝对路径，确认目标位于本项目的 `dist` 目录内；新部署尚未验证通过时不得删除上一个可用版本。
- 新正式版本完成构建、验证并上传 GitHub Release 后，本地 `dist` 只保留最新版本的部署目录、应用 ZIP，以及尚未上传过的当前 Qt 版本源码附件；删除旧版应用 ZIP、部署目录、独立校验文件及所有 QA/临时目录。GitHub 会为每个 Release 附件显示 SHA-256 摘要；已在本仓库任一 GitHub Release 保存且校验一致的 Qt 源码不得随每个应用版本重复上传。GitHub 上已发布的历史 Release 资产不受本地清理影响。
