# Obey65 三模开发进度

## 当前状态: Phase 2 - 蓝牙开发 (阻塞)

### 概览

| 阶段 | 状态 | 说明 |
|------|------|------|
| Phase 0 | ✅ 已完成 | 清理本地代码，恢复有线编译 |
| Phase 1 | ✅ 已完成 | 基础设施搭建 |
| Phase 2 | 🚫 阻塞 | 蓝牙开发 - BLE 启动代码问题 |
| Phase 3 | ⏸️ 待开始 | 2.4G 开发 |
| Phase 4 | ⏸️ 待开始 | 集成与优化 |

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

## Phase 2: 蓝牙开发 (阻塞)

### 阻塞问题

**错误报告**: [docs/errors/phase2-ble-blocker.md](../errors/phase2-ble-blocker.md)

**问题**: 当启用 `BLE_ENABLE=ON` 时，链接器找不到 `_start` 符号。

**根本原因**:
- `sdk/CMakeLists.txt` 第 56 行条件逻辑
- 当 BLE 启用时，`startup_CH583.S` 不会被包含到编译目标
- 需要 `BUILD_WIRELESS_LIB` 或修改 CMake 逻辑

**需要**:
1. 研究 WCH 官方 BLE 示例的启动流程
2. 分析其他支持 BLE 的键盘配置
3. 可能需要修改 SDK CMakeLists.txt 或创建专用启动代码

### 待完成 (等待解决阻塞问题)

- [ ] BLE 初始化与广播
- [ ] HID over GATT 服务
- [ ] 配对管理
- [ ] 多设备连接
- [ ] 蓝牙功耗优化

---

## Phase 3: 2.4G 开发 (待开始)

### 待完成

- [ ] ESB 协议实现
- [ ] 接收器固件
- [ ] 2.4G 配对流程
- [ ] 2.4G 功耗优化

---

## Phase 4: 集成与优化 (待开始)

### 待完成

- [ ] 三模切换整合
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

### 待解决

(无)

---

## 变更日志

### 2026-01-19

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
- 分析报告: [docs/analysis/project-analysis.md](../analysis/project-analysis.md)
- 开发路线图: [docs/roadmap/development-roadmap.md](../roadmap/development-roadmap.md)
- 技术文档: [docs/tech/](../tech/)

---

*最后更新: 2026-01-19*
