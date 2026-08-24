# 项目代理规则

## Windows 与 Qt 测试

- Qt 测试必须通过 CTest 运行，例如：
  `ctest --test-dir build/release --output-on-failure`。
- 禁止直接启动 `build/**/**Tests.exe`。这些测试程序依赖 CTest 注入 Qt DLL 的 `PATH`；直接运行会因找不到 `Qt6Widgets.dll` 等依赖而弹出 Windows 系统错误框。
- 非交互测试必须使用 CMake/CTest 中配置的 `QT_QPA_PLATFORM=offscreen`，不得抢占用户桌面或显示测试窗口。
- 只有用户明确要求桌面交互验收时，才启动带完整部署依赖的 `dist` 发布程序；启动前先确认不会与已有 SnipNexs 实例冲突，结束后正常退出测试实例。
- 如果必须绕过 CTest 单独诊断测试程序，先显式配置 Qt `bin` 到当前进程的 `PATH`，并确保测试窗口不会显示到用户桌面。
