# Obey65 三模键盘 - 项目分析报告

## 1. 项目架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           QMK Port CH582 项目架构                            │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                              应用层 (QMK Firmware)                           │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐│
│  │   Keymap    │ │  RGB Matrix │ │     VIA     │ │   Other QMK Features    ││
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────────────┘│
└─────────────────────────────────────────────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           适配层 (qmk_porting)                              │
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │                          protocol/                                       ││
│  │  ┌───────────────┐ ┌───────────────┐ ┌───────────────┐                  ││
│  │  │ protocol_usb.c│ │ protocol_ble.c│ │ protocol_esb.c│  ← 三模协议      ││
│  │  │   (完整)      │ │   (未实现)    │ │   (未实现)    │                  ││
│  │  └───────────────┘ └───────────────┘ └───────────────┘                  ││
│  │  ┌───────────────┐ ┌───────────────┐ ┌───────────────┐                  ││
│  │  │  protocol.c   │ │extra_keycode.c│ │usb_interface.c│                  ││
│  │  │  (协议调度)   │ │ (模式切换键)  │ │  (USB描述符)  │                  ││
│  │  └───────────────┘ └───────────────┘ └───────────────┘                  ││
│  └─────────────────────────────────────────────────────────────────────────┘│
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │                        platforms/ch58x/                                  ││
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐  ││
│  │  │ platform.c│ │  gpio.c   │ │  timer.c  │ │ eeprom/   │ │  split/   │  ││
│  │  │ (系统初始 │ │ (GPIO抽象│ │ (定时器)  │ │(EEPROM驱动│ │(分体键盘) │  ││
│  │  │    化)    │ │    层)   │ │           │ │     )     │ │           │  ││
│  │  └───────────┘ └───────────┘ └───────────┘ └───────────┘ └───────────┘  ││
│  └─────────────────────────────────────────────────────────────────────────┘│
│  ┌─────────────────────────────────────────────────────────────────────────┐│
│  │                            drivers/                                      ││
│  │  ┌─────────────────────┐ ┌─────────────────────┐                        ││
│  │  │    ws2812/          │ │    aw20216s/        │                        ││
│  │  │  (WS2812 SPI/PWM)   │ │  (LED驱动芯片)      │                        ││
│  │  └─────────────────────┘ └─────────────────────┘                        ││
│  └─────────────────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              硬件抽象层 (sdk/)                               │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐                │
│  │   BLE_LIB/      │ │ StdPeriphDriver │ │    HAL/         │                │
│  │  (BLE协议栈)    │ │  (外设驱动)     │ │  (硬件抽象)     │                │
│  │  LIBCH58xBLE.a  │ │                 │ │                 │                │
│  └─────────────────┘ └─────────────────┘ └─────────────────┘                │
│  ┌─────────────────┐ ┌─────────────────┐                                    │
│  │   USB_LIB/      │ │    LWNS/        │  ← 2.4G可用于私有协议              │
│  │  (USB协议栈)    │ │  (轻量网络)     │                                    │
│  └─────────────────┘ └─────────────────┘                                    │
└─────────────────────────────────────────────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              CH582M 硬件                                     │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐                │
│  │  RISC-V Core    │ │    BLE 5.3      │ │   2.4GHz RF     │                │
│  │    32-bit       │ │   (内置)        │ │   (内置)        │                │
│  └─────────────────┘ └─────────────────┘ └─────────────────┘                │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐                │
│  │  32KB RAM       │ │  448KB Flash    │ │  USB Device     │                │
│  └─────────────────┘ └─────────────────┘ └─────────────────┘                │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 2. 现有无线功能状态

### 2.1 项目原有无线支持分析

| 组件 | 状态 | 说明 |
|------|------|------|
| `BLE_ENABLE` 宏定义 | ✅ 存在 | common_features.cmake:449 |
| `ESB_ENABLE` 宏定义 | ✅ 存在 | common_features.cmake:463 |
| 无线协议接口 | ✅ 定义完整 | protocol.h 中的 `ch582_interface_t` |
| 模式切换键码 | ✅ 已定义 | extra_keycode.h: `BLE_SLOT0-15`, `ESB_MODE` |
| 无线库 (libXXX.a) | ❌ 私有/缺失 | 需要 `wireless/libXXX.a` 文件 |
| 蓝牙协议实现 | ❌ 未公开 | 项目依赖私有 wireless 子模块 |
| ESB协议实现 | ❌ 未公开 | 同上 |

### 2.2 Obey65 本地无线开发尝试状态

你之前创建了以下文件：

```
qmk_porting/keyboards/obey65/wireless/
├── include/
│   ├── protocol_ble.h      # BLE协议头文件 - 仅声明
│   └── protocol_esb.h      # ESB协议头文件 - 仅声明
└── src/
    ├── ble_compat.h        # BLE兼容性定义 - 补充缺失的宏
    ├── hid_dev.c           # HID服务实现 - 基本框架已完成
    ├── hid_dev.h           # HID服务头文件
    ├── protocol_ble.c      # BLE协议实现 - 部分完成
    └── protocol_esb.c      # ESB协议实现 - 仅桩代码
```

**各文件状态分析：**

| 文件 | 完成度 | 问题 |
|------|--------|------|
| `protocol_ble.c` | ~40% | 缺少：TMOS任务调度、连接管理、多设备切换、低功耗 |
| `hid_dev.c` | ~60% | 缺少：完整GATT属性表、正确的通知机制 |
| `protocol_esb.c` | ~5% | 仅桩代码，完全未实现 |
| `ble_compat.h` | ~80% | 补充了部分缺失的宏定义 |

### 2.3 当前编译问题

1. **链接问题**: "cannot find entry symbol _start" - wireless 代码破坏了链接配置
2. **依赖问题**: CMakeLists.txt 中添加了 wireless 源码，但缺少必要的函数实现
3. **接口不匹配**: `ch582_protocol_ble` 被 platform.c 引用，但实现不完整

## 3. 双灯带实现分析

### 3.1 硬件配置

Obey65 键盘使用两条独立的 WS2812 灯带：

| 灯带 | LED数量 | GPIO引脚 | 定时器 | 用途 |
|------|---------|----------|--------|------|
| 灯带1 | 50颗 | PA10 | TMR1 | 主键盘背光 (RGB Matrix) |
| 灯带2 | 4颗 | PA11 | TMR2 | 状态指示灯 |

### 3.2 实现方式

**代码位置：**
- `qmk_porting/keyboards/obey65/ws2812_tmr1.c` - 50颗灯带驱动 (TMR1+DMA)
- `qmk_porting/keyboards/obey65/ws2812_tmr2.c` - 4颗灯带驱动 (TMR2+DMA)
- `qmk_porting/keyboards/obey65/obey65.c` - 整合调用

**实现原理：**
```c
// 时序参数 (40MHz时钟)
#define PERIOD_TICKS 50    // 总周期1.25us
#define T1H_TICKS    32    // 1高电平0.8us
#define T0H_TICKS    16    // 0高电平0.4us

// DMA缓冲区: 每个LED 24bit + reset信号
static uint32_t TMR1_DmaBuf[50*24 + 80];  // 50颗LED
static uint32_t TMR2_DmaBuf[4*24 + 80];   // 4颗LED
```

**调用流程：**
```c
void ws2812_init(void) {
    tmr2_ws2812_init();  // 初始化4颗灯带
    tmr1_ws2812_init();  // 初始化50颗灯带
}

void ws2812_setleds(rgb_led_t *ledarray, uint16_t leds) {
    for (uint16_t i = 0; i < leds; i++) {
        tmr1_ws2812_update_index(i, led);  // 更新到50颗灯带
    }
}
```

### 3.3 与标准QMK WS2812驱动的差异

| 特性 | 标准驱动 | Obey65实现 |
|------|----------|------------|
| 灯带数量 | 单条 | 双条 |
| 驱动方式 | SPI/PWM | TMR+DMA |
| 引脚配置 | 单GPIO | 双GPIO (PA10, PA11) |
| QMK集成 | 完全兼容 | 自定义ws2812_setleds |

## 4. 依赖分析

### 4.1 蓝牙开发所需SDK组件

| 组件 | 路径 | 用途 |
|------|------|------|
| BLE库 | `sdk/BLE_LIB/LIBCH58xBLE.a` | BLE协议栈（二进制） |
| BLE头文件 | `sdk/BLE_LIB/CH58xBLE_LIB.H` | API定义 |
| HAL | `sdk/HAL/` | 硬件初始化、TMOS调度 |
| 外设驱动 | `sdk/StdPeriphDriver/` | GPIO、定时器等 |

**关键API：**
```c
// TMOS任务调度
uint8_t TMOS_ProcessEventRegister(pTaskEventHandlerFn handler);
void TMOS_SystemProcess(void);

// GAP角色
void GAPRole_PeripheralInit(void);
bStatus_t GAPRole_SetParameter(uint16_t param, uint8_t len, void *pValue);
bStatus_t GAPRole_PeripheralStartDevice(...);

// GATT服务
bStatus_t GATTServApp_RegisterService(...);
bStatus_t GATT_Notification(uint16_t connHandle, attHandleValueNoti_t *pNoti, uint8_t authenticated);
```

### 4.2 2.4G开发所需组件

| 组件 | 说明 |
|------|------|
| RF驱动 | CH582内置2.4G RF，可通过BLE库或LWNS访问 |
| ESB协议 | 需要自行实现类似Nordic ESB的私有协议 |
| LWNS | SDK提供的轻量网络栈，可用于2.4G通信 |

### 4.3 接收器固件所需

- 独立的CH582板（或其他USB芯片）
- USB HID Device实现
- 与键盘端匹配的2.4G协议
- 配对机制

## 5. 风险评估

### 5.1 技术风险

| 风险 | 级别 | 说明 | 缓解措施 |
|------|------|------|----------|
| BLE库是闭源二进制 | 高 | 无法调试内部问题 | 参考WCH官方示例，严格按API使用 |
| 无线库未公开 | 高 | 无法直接参考现有实现 | 分析protocol接口，自行实现 |
| 内存受限(32KB RAM) | 中 | BLE协议栈占用较多RAM | 优化数据结构，减少缓冲区 |
| 无串口调试 | 中 | 调试困难 | 实现键盘输入调试系统 |
| 双灯带兼容性 | 低 | 三模可能影响RGB | 确保模式切换不干扰DMA |

### 5.2 当前代码问题

1. **rules.cmake配置冲突**
   - 同时启用了 `BLE_ENABLE=ON` 和 `ESB_ENABLE=ON`
   - 但 `ESB_ROLE` 未定义，导致链接时找不到符号

2. **CMakeLists.txt修改风险**
   - 添加了 `BUILD_WIRELESS_FROM_SOURCE` 逻辑
   - 可能与原有的wireless库加载逻辑冲突

3. **platform.c引用问题**
   - 代码引用了 `ch582_protocol_ble`
   - 但该变量在自定义代码中的实现可能不完整

### 5.3 建议的清理方案

```cmake
# keymaps/default/rules.cmake - 建议修改
set(BLE_ENABLE OFF CACHE BOOL "KB" FORCE)  # 先禁用蓝牙
set(ESB_ENABLE OFF CACHE BOOL "KB" FORCE)  # 先禁用2.4G
# 等有线模式稳定后再逐步启用
```

## 6. 参考资源

### 6.1 WCH官方示例

- CH583EVT: https://github.com/openwch/ch583/tree/main/EVT/EXAM/BLE
- HID Keyboard示例: `BLE/HID_Keyboard/`
- 直接传输示例: `BLE/Direct_Test/`

### 6.2 项目中可参考的键盘

| 键盘 | 无线状态 | 参考价值 |
|------|----------|----------|
| mk01 | BLE+ESB已启用 | rules.cmake配置参考 |
| mk02 | BLE+ESB已启用 | 同上 |
| m2wired | 仅有线 | 纯有线baseline |

### 6.3 协议接口定义

关键接口在 `protocol.h`:
```c
typedef struct _ch582_interface_t {
    host_driver_t ch582_common_driver;  // QMK驱动接口
    ch582_driver_t ch582_platform_initialize;
    ch582_driver_t ch582_protocol_setup;
    ch582_driver_t ch582_protocol_init;
    ch582_driver_t ch582_protocol_pre_task;
    ch582_driver_t ch582_protocol_post_task;
    ch582_driver_t ch582_platform_run;
    ch582_driver_t ch582_platform_reboot;
} ch582_interface_t;
```

---

*文档生成日期: 2026-01-19*
*作者: Claude Code*
