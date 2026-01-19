# Obey65 三模开发进度

## 当前状态: Phase 4 - 集成与优化 (待开始)

### 概览

| 阶段 | 状态 | 说明 |
|------|------|------|
| Phase 0 | ✅ 已完成 | 清理本地代码，恢复有线编译 |
| Phase 1 | ✅ 已完成 | 基础设施搭建 |
| Phase 2 | ✅ 已完成 | 蓝牙开发 |
| Phase 3 | ✅ 已完成 | 2.4G ESB 开发 |
| Phase 4 | 🔄 进行中 | 集成与优化 |

---

## Phase 0: 现有架构清理 ✅

**完成日期**: 2026-01-19
**总结文档**: [docs/summary/phase0-summary.md](../summary/phase0-summary.md)

### 主要成果

- [x] 项目分析与文档生成
- [x] 禁用 BLE/ESB 配置 (commit: 677699b9)
- [x] 保留 wireless 代码作为参考 (commit: ec7f4ddd)
- [x] 修复 Python 依赖 (click, cryptography, PyYAML)
- [x] 验证编译成功，提交稳定基线 (commit: 7a4cc4df)

---

## Phase 1: 基础设施搭建 ✅

**完成日期**: 2026-01-19
**总结文档**: [docs/summary/phase1-summary.md](../summary/phase1-summary.md)

### 主要成果

- [x] **Phase 1.1: 调试基础设施** (commit: dfe641f4)
  - [x] UART2 串口调试输出 (PB23)
  - [x] 调试宏和条件编译 (DEBUG_UART_ENABLE)
  - [x] 115200 baud 输出

- [x] **Phase 1.2: 协议抽象层** (commit: c9d45bb6)
  - [x] wireless_mode.h/c 模式管理
  - [x] USB/BLE/ESB 状态机
  - [x] 4 个 BLE 配对槽位

- [x] **Phase 1.3: 电池检测基础** (commit: 5ef59d66)
  - [x] ADC 采样 (PA4)
  - [x] 电压/百分比计算
  - [x] 充电状态检测 (PB12)

- [x] **Phase 1.4: 电源管理框架** (commit: 5ece2576)
  - [x] 5 种功耗模式定义
  - [x] 自动超时切换
  - [x] 唤醒源配置

### 固件状态

```
Memory region         Used Size  Region Size  %age Used
       FLASH:      105756 B       372 KB     27.76%
         RAM:       24160 B        32 KB     73.73%
```

---

## Phase 2: 蓝牙开发 ✅

**完成日期**: 2026-01-19
**总结文档**: [docs/summary/phase2-summary.md](../summary/phase2-summary.md)

### 阻塞问题 - 已解决 ✅

**原错误报告**: [docs/errors/phase2-ble-blocker.md](../errors/phase2-ble-blocker.md)

**问题**: 当启用 `BLE_ENABLE=ON` 时，链接器找不到 `_start` 符号。

**根本原因**:
- `sdk/CMakeLists.txt` 第 56 行条件逻辑
- 当 BLE 启用时，`startup_CH583.S` 不会被包含到编译目标
- `BUILD_WIRELESS_FROM_SOURCE` 变量未被考虑

**解决方案** (commit: e646b2b4):
1. 修改 `sdk/CMakeLists.txt`，添加 `BUILD_WIRELESS_FROM_SOURCE` 条件
2. 添加 `SLEEP.c` 到编译目标
3. 创建 `ble_support.c` 提供必需的变量定义:
   - `MEM_BUF`: BLE 协议栈内存缓冲区
   - `MacAddr`: MAC 地址定义
   - 无线库接口桩函数

### 固件状态

```
Memory region         Used Size  Region Size  %age Used
       FLASH:      231612 B       372 KB     60.80%
         RAM:       28340 B        32 KB     86.49%
```

### Phase 2 任务进度

- [x] **Phase 2.0: BLE 编译修复** (commit: e646b2b4)
  - [x] 修复 CMake 启动代码条件
  - [x] 添加 SLEEP.c 支持
  - [x] 添加 BLE 支持桩函数

- [x] **Phase 2.1: BLE 初始化与广播** (commit: 3ec100e0)
  - [x] GAP Peripheral Role 配置
  - [x] 状态变化回调实现
  - [x] 配对状态和密码回调
  - [x] 广播数据和扫描响应配置
  - [x] 低延迟连接参数 (7.5ms-15ms)

- [x] **Phase 2.2: HID over GATT 服务** (commit: a861c6b7)
  - [x] 完善 HID Report Map (Keyboard/Mouse/Consumer/System)
  - [x] 实现键盘报告发送
  - [x] 实现 LED 状态接收 (HidDev_GetKeyboardLeds)
  - [x] Consumer Control 支持 (媒体键)
  - [x] System Control 支持 (电源/睡眠)
  - [x] Boot Protocol 属性

- [x] **Phase 2.3: 配对管理** (commit: a71b47b9)
  - [x] 实现配对流程 (GAPBondMgr 集成)
  - [x] 实现绑定信息存储 (ble_bonding.c)
  - [x] 实现配对槽位切换 (ble_switch_slot)
  - [x] 清除所有配对 (ble_clear_all_bonds)

- [x] **Phase 2.4: 多设备连接** (commit: c327f177)
  - [x] 4 设备槽位管理 (BLE_MAX_BONDS=4)
  - [x] 设备切换逻辑完善
  - [x] 与 wireless_mode 模块集成

- [x] **Phase 2.5: 蓝牙功耗优化** (commit: fe3fb195)
  - [x] BLE 连接状态感知的睡眠控制
  - [x] 动态连接参数调整 (低延迟/省电模式)
  - [x] 集成电源管理与 BLE 功耗模式

---

## Phase 3: 2.4G ESB 开发 ✅

**完成日期**: 2026-01-19

### Phase 3 任务进度

- [x] **Phase 3.0-3.3: ESB 协议基础实现** (commit: 981a8f70)
  - [x] RF 初始化 (2Mbps, Auto ACK)
  - [x] 消息类型定义 (配对/HID/ACK/心跳)
  - [x] 键盘/鼠标/媒体键报告发送
  - [x] ACK/重传机制 (3次，10ms超时)
  - [x] 配对协议 (广播请求/响应/确认)
  - [x] 心跳检测 (500ms，3次失败断连)
  - [x] 频率跳变 (16信道)

- [x] **Phase 3.4: ESB 功耗优化** (commit: 5dd65005)
  - [x] 集成 power_mode 模块
  - [x] 空闲时进入低功耗模式
  - [x] 活动时退出低功耗模式

- [x] **Phase 3.5: 接收器固件设计**
  - [x] 创建 obey65_receiver 键盘定义
  - [x] 实现 RF 接收状态机
  - [x] 配对流程处理
  - [x] LED 状态指示
  - [x] USB HID 集成接口

---

## Phase 4: 集成与优化 (进行中)

### Phase 4 任务进度

- [x] **Phase 4.1: 三模切换逻辑** (commit: 8fec50ef)
  - [x] USB 自动检测 (插入 USB 自动切换)
  - [x] USB 拔出恢复之前无线模式
  - [x] 模式切换按键码定义 (WL_USB/WL_ESB/WL_BLE0-3)
  - [x] 防抖处理 (500ms)

- [ ] **Phase 4.2: 统一电源管理**
  - [ ] 充电时行为定义
  - [ ] 模式间电源状态协调

- [x] **Phase 4.3: LED 状态指示** (commit: 8fec50ef)
  - [x] 新增 status_indicator 模块
  - [x] 模式颜色指示 (USB=蓝/BLE=青/ESB=绿)
  - [x] 连接状态指示 (常亮/闪烁/暗淡)
  - [x] 电池电量指示 (4 LED)
  - [x] 自动切换显示模式

### 待完成

- [ ] VIA 无线配置
- [ ] 整体功耗测试
- [ ] 稳定性测试

---

## 问题记录

### 已解决

#### Issue #1: 链接器找不到 _start 符号 (Phase 0)
**解决**: 禁用 BLE_ENABLE 和 ESB_ENABLE

#### Issue #2: Python 依赖缺失 (Phase 0)
**解决**: `pip3 install click cryptography PyYAML --user`

#### Issue #3: _putchar 多重定义 (Phase 1.1)
**解决**: 移除 debug_uart.c 中的 _putchar 定义，使用平台定义

#### Issue #4: rgbled_power_off 未定义 (Phase 1.3)
**解决**: 包含 rgb_led.h 头文件

#### Issue #5: RB_PWR_RAM16K 未定义 (Phase 1.4)
**解决**: 使用 RB_PWR_RAM30K (SDK 实际定义)

#### Issue #6: BLE 编译阻塞 - _start 符号缺失 (Phase 2)
**解决**: 修改 sdk/CMakeLists.txt 添加 BUILD_WIRELESS_FROM_SOURCE 条件，
添加 SLEEP.c 和 ble_support.c (commit: e646b2b4)

### 待解决

(无)

---

## 变更日志

### 2026-01-19

**Phase 3.5: 接收器固件设计**
- 创建 obey65_receiver 键盘定义目录
- 实现 esb_receiver.c/h (RF 接收、配对、状态机)
- 实现 obey65_receiver.c (主函数、USB 集成)
- 创建 qmk_config.h, halconf.h, mcuconf.h
- 创建 debug_uart.c/h, info.json
- 更新 Phase 3 总结文档

**Phase 4.1 & 4.3: 三模切换与状态指示** (commit: 8fec50ef)
- USB 自动检测和模式切换
- 模式切换按键码定义
- status_indicator 模块
- RGB LED 模式/电量/连接状态指示

**Phase 2.5: 蓝牙功耗优化** (commit: fe3fb195)
- BLE 连接状态感知的睡眠控制
- 动态连接参数调整 (7.5-15ms / 30-50ms)
- power_mode 与 BLE 功耗模式集成
- 睡眠时保持 BLE 单元供电 (RB_PWR_EXTEND)

**Phase 2.4: 多设备连接** (commit: c327f177)
- wireless_mode 模块与 BLE 集成
- ble_switch_slot 函数调用
- 模式切换时正确处理 BLE 连接

**Phase 2.3: 配对管理** (commit: a71b47b9)
- 新增 ble_bonding.c/h 配对槽位管理
- 支持 4 个配对槽位切换
- 集成 GAPBondMgr 配对数据存储
- 添加清除配对功能

**Phase 2.2: HID over GATT 服务** (commit: a861c6b7)
- 添加 Consumer Control 和 System Control 报告
- 完善 HID Report Map 描述符
- 修复连接句柄管理
- 实现 LED 状态获取

**Phase 2.1: BLE 初始化与广播** (commit: 3ec100e0)
- 实现 GAP Peripheral Role 配置
- 实现状态变化/配对/密码回调
- 配置广播数据和扫描响应
- 设置低延迟连接参数

**Phase 2 阻塞解决**
- 修复 BLE 编译问题 (commit: e646b2b4)
  - sdk/CMakeLists.txt: 添加 BUILD_WIRELESS_FROM_SOURCE 条件
  - sdk/CMakeLists.txt: 添加 SLEEP.c 到编译目标
  - 新文件: ble_support.c (MEM_BUF, MacAddr, 桩函数)

**Phase 1 完成**

- **Phase 1.1**: 调试 UART 基础设施
  - commit: dfe641f4
  - 新文件: debug_uart.h, debug_uart.c

- **Phase 1.2**: 无线模式管理框架
  - commit: c9d45bb6
  - 新文件: wireless_mode.h, wireless_mode.c

- **Phase 1.3**: 电池管理模块
  - commit: 5ef59d66
  - 新文件: battery.h, battery.c

- **Phase 1.4**: 电源管理框架
  - commit: 5ece2576
  - 新文件: power_mode.h, power_mode.c

**Phase 0 完成**
- 清理 rules.cmake 配置
- 保留 wireless 代码作为开发参考
- 修复编译环境 (Python 依赖)
- 验证编译成功，建立稳定基线

---

## 资源链接

- 项目仓库: (本地)
- Phase 0 总结: [docs/summary/phase0-summary.md](../summary/phase0-summary.md)
- Phase 1 总结: [docs/summary/phase1-summary.md](../summary/phase1-summary.md)
- Phase 3 总结: [docs/summary/phase3-summary.md](../summary/phase3-summary.md)
- 分析报告: [docs/analysis/project-analysis.md](../analysis/project-analysis.md)
- 开发路线图: [docs/roadmap/development-roadmap.md](../roadmap/development-roadmap.md)
- 技术文档: [docs/tech/](../tech/)

---

*最后更新: 2026-01-19*
