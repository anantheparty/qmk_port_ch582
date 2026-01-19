# Obey65 三模键盘固件开发 - 初始化 Prompt

本文件位于：./InitPrompt.md 欢迎随时阅读

## 项目背景

我正在开发一个名为 **Obey65** 的 65% 布局机械键盘，使用 **CH582M** 芯片（RISC-V + BLE 5.3）。

当前项目基于 `qmk_port_ch582` 开源项目，这是一个将 QMK 固件移植到 CH582 平台的适配层。

### 当前状态
- ✅ **有线模式**: 已完成，USB HID 正常工作
- ✅ **双灯带 RGB**: 已完成，使用两个独立 GPIO 引脚控制两条灯带
- ✅ **VIA 支持**: 已完成配置
- ❌ **蓝牙模式**: 未开发
- ❌ **2.4G 模式**: 未开发
- ❌ **2.4G 接收器固件**: 未开发

### 硬件特殊性
1. **双灯带设计**: 键盘有两条独立的 WS2812 灯带，由两个不同的 GPIO 引脚控制（原驱动只支持单灯带）
2. **无串口调试**: PCB 没有预留串口引脚，调试需要通过键盘输入（打字输出）方式进行
3. **CH582M 芯片**: 内置 BLE 5.3 和 2.4GHz RF，32KB RAM，448KB Flash

---

## 开发目标

完成 Obey65 键盘的**三模**功能开发：
1. **有线模式** (USB) - 已完成
2. **蓝牙模式** (BLE HID) - 待开发
3. **2.4G 模式** (私有协议) - 待开发，需要同时开发接收器固件

---

## 第一步：请先阅读并理解项目

请使用 MCP filesystem 工具完整阅读以下内容：

### 1. 项目整体结构
```
先执行: ls -la 查看根目录
然后阅读:
- README.md 和 README.zh-cn.md
- CMakeLists.txt (顶层构建配置)
```

### 2. 我的键盘定义
```
qmk_porting/keyboards/obey65/
- config.h (特别关注双灯带的引脚配置)
- keymap 目录
- 所有相关配置文件
```

### 3. 核心适配层
```
qmk_porting/
- common_features.cmake (功能开关，重点关注 BLE_ENABLE, ESB_ENABLE 相关)
- platforms/ch58x/ (平台驱动)
- protocol/ (协议实现，关注现有的蓝牙/无线相关代码)
```

### 4. WCH SDK 相关
```
sdk/ 目录，特别是:
- BLE 相关的头文件和库
- RF 相关的代码
```

### 5. 现有无线相关代码（即使不完整）
```
搜索所有包含以下关键词的文件:
- BLE, bluetooth, wireless
- ESB, RF, 2.4G, dongle
- WIRELESS_ENABLE, BLE_ENABLE
```

---

## 阅读完成后，请输出以下内容

### A. 项目分析报告 (`docs/analysis/project-analysis.md`)

包含：
1. **项目架构图** (用 Mermaid 或 ASCII)
2. **现有无线功能状态**：哪些代码已实现、哪些是桩代码、哪些完全缺失
3. **双灯带实现分析**：当前如何实现的，代码位置
4. **依赖分析**：蓝牙和 2.4G 开发需要哪些 WCH SDK 组件
5. **风险评估**：可能遇到的技术难点

### B. 开发路线图 (`docs/roadmap/development-roadmap.md`)

分阶段的详细开发计划：

#### Phase 0: 现有架构清理 (预计 X 天)
- 我之前进行了一些开发尝试，但是目前编译都通过不了，git的上一个云端版本是单模稳定可用的版本
- 分析本地文件，清理文件
- 必要时，可舍弃本地文件

#### Phase 1: 基础设施 (预计 X 天)
- 调试系统搭建（键盘输入调试）
- 电源管理基础
- 模式切换框架

#### Phase 2: 蓝牙开发 (预计 X 天)
- BLE HID 服务实现
- 配对/绑定逻辑
- 多设备切换
- 低功耗优化

#### Phase 3: 2.4G 开发 (预计 X 天)
- 私有协议设计
- 键盘端发射代码
- 接收器固件开发
- 配对协议

#### Phase 4: 集成与优化 (预计 X 天)
- 三模切换逻辑
- 统一电源管理
- LED 状态指示
- 全面测试

每个阶段包含：
- 具体任务清单（需要将每个阶段拆解成若干任务，若该阶段依赖前置开发结果，不好拆解，可以暂设为未拆解）
- 验收标准
- 测试方法
- 预期的 Git 提交点

对于阶段和阶段的拆分，完整的记录在路线图文档中，并动态更新

### C. 技术方案文档 (`docs/tech`)

需要的文档：
1. `bluetooth-hid-design.md` - 蓝牙 HID 实现方案
2. `2.4g-protocol-design.md` - 2.4G 私有协议设计
3. `dongle-firmware-design.md` - 接收器固件设计
4. `power-management.md` - 电源管理方案
5. `debug-system.md` - 键盘输入调试系统设计

### D. 开发进度跟踪 (`docs/progress/PROGRESS.md`)

模板：
```markdown
# Obey65 三模开发进度

## 当前状态: Phase X - [阶段名称]

### 已完成
- [ ] 任务1 (commit: xxx)
- [ ] 任务2 (commit: xxx)

### 进行中
- [ ] 任务3

### 待开始
- [ ] 任务4

## 变更日志
### [日期]
- 完成了 xxx
- 发现问题: xxx
- 下一步: xxx
```

---

## 开发规范要求

### Git 提交规范
每完成一个功能点就提交，格式：
```
feat(bluetooth): implement BLE HID service
fix(rgb): fix dual strip sync issue
docs(roadmap): update phase 2 progress
test(debug): add keyboard input debug module
```

### 代码规范
- 新增代码添加详细注释（中英文均可）
- 复杂逻辑添加设计说明
- 保持与现有代码风格一致

### 调试输出规范
由于没有串口，实现一个通过键盘输入的调试系统：
```c
// 示例：按下特定组合键时，键盘会"打出"调试信息
// Fn + D + 1 -> 输出当前连接状态
// Fn + D + 2 -> 输出电池电量
// Fn + D + 3 -> 输出蓝牙状态
```

### 测试规范
每个功能模块需要：
1. 单元测试方案（如果可行）
2. 手动测试步骤
3. 预期结果
4. 实际结果记录

---

## 重要提醒

1. **不要直接修改 qmk_firmware 子模块**，所有改动应在 qmk_porting 层
2. **WCH 蓝牙库是二进制 blob**，需要参考 openwch/ch583 官方示例
3. **2.4G 可能需要使用 ESB 类似的私有协议**，参考 common_features.cmake 中的 ESB_ENABLE
4. **接收器固件**需要单独的构建目标，可能是另一个 CH582 或简单的 USB 芯片
5. **保持双灯带功能正常**，三模开发不应影响现有的 RGB 实现

---

## 开始吧！

在开始前，先整理本地claudecode相关文件，并进行首次git提交，将claudecode准备文件skill mcp等文件全部上传，包括本文件

然后完成项目阅读和分析，输出上述文档。在开始实际编码之前，我们需要：
1. 确认你对项目的理解是否正确
2. 讨论技术方案的可行性
3. 确定优先级和时间估算
4. 及时更新CLAUDE.md 那是你的外置储存和sketchbook

准备好后，请开始阅读项目并生成分析报告。