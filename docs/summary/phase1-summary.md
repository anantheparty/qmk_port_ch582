# Phase 1 总结: USB 基础与调试框架

## 完成日期
2026-01-19

## 阶段目标
建立 Obey65 三模键盘的基础框架，包括调试基础设施、协议抽象层、电池检测和电源管理。

## 完成的任务

### Phase 1.1: 调试基础设施 - 串口输出 ✅
**提交**: `dfe641f4`

- 选择 UART2 + 备用引脚 (PB22/PB23) 避免矩阵引脚冲突
- 创建 `debug_uart.h/c` 模块
- 实现调试宏 `DEBUG_INIT()`, `DEBUG_PRINT()`, `DEBUG_PRINTF()`
- 通过 CMake 选项 `DEBUG_UART_ENABLE` 可选启用
- 波特率: 115200 baud

**文件**:
- `qmk_porting/keyboards/obey65/debug_uart.h`
- `qmk_porting/keyboards/obey65/debug_uart.c`
- `qmk_porting/keyboards/obey65/halconf.h` (启用 UART2 重映射)

### Phase 1.2: 协议抽象层重构 ✅
**提交**: `c9d45bb6`

- 创建 `wireless_mode.h/c` 实现无线模式状态机
- 定义三种传输模式: USB, BLE, ESB
- 支持 4 个 BLE 配对槽位
- 定义连接状态: DISCONNECTED, CONNECTING, CONNECTED, ERROR
- 集成调试输出

**文件**:
- `qmk_porting/keyboards/obey65/wireless_mode.h`
- `qmk_porting/keyboards/obey65/wireless_mode.c`

### Phase 1.3: 电池检测基础 ✅
**提交**: `5ef59d66`

- 创建 `battery.h/c` 实现电池状态管理
- 自定义 `battery_map[]` 适配 Obey65 硬件电压分压
- 实现 `battery_critical_gpio_prerequisite()` 平台回调
- 支持电池百分比、电压、充电状态查询
- 集成 RGB LED 关闭功能用于低电量保护

**硬件配置**:
- 电池电压检测: PA4 (ADC CH4)
- 充电检测: PB12 (POWER_DETECT_PIN)
- 分压比: 1:1 (Vbat/2)

**文件**:
- `qmk_porting/keyboards/obey65/battery.h`
- `qmk_porting/keyboards/obey65/battery.c`

### Phase 1.4: 电源管理框架 ✅
**提交**: `5ece2576`

- 创建 `power_mode.h/c` 实现功耗模式管理
- 定义 5 种功耗模式:
  - ACTIVE: 全性能模式
  - NORMAL: 正常模式 (降低扫描率)
  - IDLE: 空闲模式 (RGB 关闭)
  - SLEEP: 睡眠模式 (CPU 休眠)
  - DEEP_SLEEP: 深度睡眠 (最低功耗)
- 集成 CH58x SDK 低功耗 API
- 配置 GPIO/USB/BAT 唤醒源
- 在 `obey65.c` 中集成 `housekeeping_task_kb()`

**默认超时配置**:
| 模式 | 超时时间 |
|------|----------|
| NORMAL | 5 秒 |
| IDLE | 30 秒 |
| SLEEP | 5 分钟 |
| DEEP_SLEEP | 30 分钟 |

**文件**:
- `qmk_porting/keyboards/obey65/power_mode.h`
- `qmk_porting/keyboards/obey65/power_mode.c`
- `qmk_porting/keyboards/obey65/obey65.c` (更新)

## 编译结果

```
Memory region         Used Size  Region Size  %age Used
       FLASH:      105756 B       372 KB     27.76%
         RAM:       24160 B        32 KB     73.73%

obey65_IAP.elf:
       FLASH:       33980 B        72 KB     46.09%
         RAM:       17164 B        32 KB     52.38%
```

## 架构图

```
┌─────────────────────────────────────────────────────────┐
│                      obey65.c                           │
│  keyboard_post_init_kb()  housekeeping_task_kb()       │
└───────────────┬───────────────────────┬─────────────────┘
                │                       │
    ┌───────────▼───────────┐   ┌───────▼───────────┐
    │   wireless_mode.c     │   │   power_mode.c    │
    │   - USB/BLE/ESB       │   │   - 5 功耗模式     │
    │   - 模式切换          │   │   - 自动休眠       │
    │   - 状态管理          │   │   - 唤醒源配置     │
    └───────────┬───────────┘   └───────┬───────────┘
                │                       │
    ┌───────────▼───────────┐   ┌───────▼───────────┐
    │     battery.c         │   │    debug_uart.c   │
    │   - ADC 采样          │   │   - UART2 输出    │
    │   - 电量计算          │   │   - 调试日志      │
    │   - 充电状态          │   │   - 条件编译      │
    └───────────────────────┘   └───────────────────┘
```

## 文件变更统计

| 文件 | 新增行数 | 描述 |
|------|----------|------|
| debug_uart.h | 42 | 调试 UART 接口 |
| debug_uart.c | 107 | UART2 实现 |
| wireless_mode.h | 67 | 无线模式接口 |
| wireless_mode.c | 112 | 模式状态机 |
| battery.h | 30 | 电池接口 |
| battery.c | 124 | 电池管理 |
| power_mode.h | 48 | 电源模式接口 |
| power_mode.c | 270 | 功耗管理 |
| rules.cmake | +20 | 构建配置 |
| obey65.c | +25 | 集成代码 |

**总计**: 新增约 845 行代码

## 已知问题与限制

1. **电池检测校准**: `battery_map[]` 需要根据实际硬件进行校准
2. **功耗模式验证**: 需要实际测量各模式下的功耗
3. **唤醒延迟**: 从 SLEEP 模式唤醒可能有延迟，需测试
4. **unused 函数警告**: `enter_idle_mode()` 预留未使用

## 下一阶段计划

### Phase 2: 蓝牙开发
- 2.1: BLE 初始化与广播
- 2.2: HID over GATT 配置
- 2.3: 配对管理
- 2.4: 多设备连接
- 2.5: 功耗优化

## 测试建议

1. **调试串口**: 连接 PB23 (TX) 到 USB-TTL 转换器验证调试输出
2. **电池检测**: 用可调电源模拟电池电压，验证百分比计算
3. **电源模式**: 监测 RGB LED 状态变化验证模式切换

---

*生成日期: 2026-01-19*
