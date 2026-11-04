# v0.8.5 本地候选包验证

日期：2026-11-04。本次将应用设置改为 INI 文件；没有创建版本标签或 GitHub Release。

## 配置行为

- 可写便携目录：设置保存为 `bin/SnipNexs.ini`，与 EXE 同目录，首次保存时创建。
- 程序目录不可写且没有现有 INI：使用平台指定的当前用户本地数据目录下的 `SnipNexs/SnipNexs.ini`。本机 Qt 6.11.2 测试返回 `AppData/LocalLow`；常规路径也可能为 `AppData/Local`，不硬编码用户名或盘符。
- 语言、录屏音源、模型镜像与启动更新选项统一使用显式 INI 后端，不读取或写入应用注册表配置，不迁移旧设置。
- 已有便携 INI 始终优先；不能写入时明确显示路径和错误提示。设置窗口保持打开，可以重试或取消。
- 本机真实当前用户的 `HKCU/Software/SnipNexs/SnipNexs` 在本轮验证前后均不存在。

## 验证结果

- Release 构建成功。CTest 共 32 项：26 通过、6 跳过、0 失败。
- 新增文件存储测试通过：显式 INI 格式、便携路径、写入后重读、独立存储、不可写程序目录的回退、既有配置路径冲突时报告失败。
- 设置窗口通过实际按钮保存录屏音源、模型镜像和更新选项；失败场景验证提示及窗口保持打开。全部采用 offscreen，经 CTest 运行。
- 现有 OCR、真实 Hy-MT2 推理、界面、截图/录屏核心及打包回归通过。部署后的启动自检和运行库自检 2 项均通过，使用隔离 PATH。
- 跳过：NativeTrayIconTests、CaptureExclusionTests、NativeScreenRecorderTests、AudioCaptureTests、MicrophoneCaptureTests、TranslationModelDownloadTests。未重复进行桌面/音频实机、干净 Windows 或安装器实装验收。

## 交付与收尾

完整解压 `dist/SnipNexs-0.8.5-win64.zip` 后运行 `bin/SnipNexs.exe`，不要单独复制 EXE。

- ZIP：1,130,973,089 字节，44 个文件；不含个人 INI、历史截图或测试素材。
- ZIP SHA-256：`59cf90ccb3e73defcc6b2c084409a81c8ae48b9085da7a64db21e988c70fafdb`。
- EXE SHA-256：`6c19a4da3116f1e16320cec45d5c7faee2cd7015184673bfc66def99d7a2be97`。
- 唯一本地翻译引擎仍为 Hy-MT2；离线模型哈希保持 `dc5f44fcf1fa496ee7ad725982c0c8c553a4de00259b53af84c4b89fb0c06699`。
- ZIP 每个文件均与部署内容核对。新包验证成功后已退役 0.8.4 部署目录，保留回退 ZIP；原历史哈希不变。旧 manifest 留在 `user-data/SnipNexs-0.8.4-win64` 供恢复。

详细本地证据保存在 `artifacts/package-v0.8.5.json`、`artifacts/validation-v0.8.5/` 及对应清理记录，不提交机器日志和截图到仓库。
