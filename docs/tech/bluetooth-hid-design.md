# 蓝牙 HID 实现方案

## 1. 概述

本文档描述 Obey65 键盘的 BLE HID 实现方案，基于 CH582 芯片的 BLE 5.3 协议栈。

## 2. 架构设计

### 2.1 模块划分

```
┌─────────────────────────────────────────────────────────────────┐
│                        应用层                                    │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                │
│  │  Keymap     │ │  Mode Switch│ │  Battery    │                │
│  └─────────────┘ └─────────────┘ └─────────────┘                │
└─────────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                     BLE 协议实现层                               │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    protocol_ble.c                            ││
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐    ││
│  │  │  GAP      │ │  GATT     │ │  HID Svc  │ │  Battery  │    ││
│  │  │ Peripheral│ │  Server   │ │  (hid_dev)│ │  Service  │    ││
│  │  └───────────┘ └───────────┘ └───────────┘ └───────────┘    ││
│  └─────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    CH582 BLE 协议栈                              │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                │
│  │  TMOS       │ │  Link Layer│ │  RF Driver  │                │
│  │  Scheduler  │ │             │ │             │                │
│  └─────────────┘ └─────────────┘ └─────────────┘                │
│                    LIBCH58xBLE.a                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 核心接口

实现 `ch582_interface_t` 接口：

```c
const ch582_interface_t ch582_protocol_ble = {
    .ch582_common_driver.keyboard_leds = ble_keyboard_leds,
    .ch582_common_driver.send_keyboard = ble_send_keyboard,
    .ch582_common_driver.send_nkro = ble_send_nkro,
    .ch582_common_driver.send_mouse = ble_send_mouse,
    .ch582_common_driver.send_extra = ble_send_extra,
    .ch582_platform_initialize = ble_platform_initialize,
    .ch582_protocol_setup = ble_protocol_setup,
    .ch582_protocol_init = ble_protocol_init,
    .ch582_protocol_pre_task = ble_protocol_pre_task,
    .ch582_protocol_post_task = ble_protocol_post_task,
    .ch582_platform_run = ble_platform_run,
    .ch582_platform_reboot = ble_platform_reboot,
};
```

## 3. 详细设计

### 3.1 BLE 配置

```c
// BLE 配置参数
#define BLE_DEVICE_NAME         "Obey65"
#define BLE_TX_POWER            LL_TX_POWEER_0_DBM  // 0dBm
#define BLE_ADV_INTERVAL_MIN    80   // 100ms (80 * 1.25ms)
#define BLE_ADV_INTERVAL_MAX    160  // 200ms
#define BLE_CONN_INTERVAL_MIN   6    // 7.5ms  (快速响应)
#define BLE_CONN_INTERVAL_MAX   16   // 20ms   (省电模式)
#define BLE_SLAVE_LATENCY       30   // 允许跳过30个连接事件
#define BLE_CONN_TIMEOUT        500  // 5秒超时
#define BLE_SLOT_COUNT          4    // 支持4个设备槽位
```

### 3.2 GATT 服务结构

```
GATT Server
├── Generic Access Service (0x1800)
│   ├── Device Name (0x2A00)
│   └── Appearance (0x2A01)  -> Keyboard
├── Generic Attribute Service (0x1801)
├── HID Service (0x1812)
│   ├── HID Information (0x2A4A)
│   ├── HID Control Point (0x2A4C)
│   ├── Protocol Mode (0x2A4E)
│   ├── Report Map (0x2A4B)
│   ├── Report: Keyboard Input (0x2A4D) - ID 1, Type Input
│   ├── Report: Keyboard Output (0x2A4D) - ID 1, Type Output (LEDs)
│   ├── Report: Mouse Input (0x2A4D) - ID 2, Type Input
│   └── Report: Consumer Input (0x2A4D) - ID 3, Type Input
├── Battery Service (0x180F)
│   └── Battery Level (0x2A19)
└── Device Information Service (0x180A)
    ├── Manufacturer Name (0x2A29)
    └── PnP ID (0x2A50)
```

### 3.3 HID Report Map

```c
static const uint8_t hidReportMap[] = {
    // Keyboard
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report ID (1)

    // Modifier keys
    0x05, 0x07,       //   Usage Page (Keyboard)
    0x19, 0xE0,       //   Usage Minimum (Left Control)
    0x29, 0xE7,       //   Usage Maximum (Right GUI)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)

    // Reserved byte
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x08,       //   Report Size (8)
    0x81, 0x01,       //   Input (Constant)

    // LEDs
    0x95, 0x05,       //   Report Count (5)
    0x75, 0x01,       //   Report Size (1)
    0x05, 0x08,       //   Usage Page (LEDs)
    0x19, 0x01,       //   Usage Minimum (Num Lock)
    0x29, 0x05,       //   Usage Maximum (Kana)
    0x91, 0x02,       //   Output (Data, Variable, Absolute)
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x03,       //   Report Size (3)
    0x91, 0x01,       //   Output (Constant)

    // Key array (6KRO)
    0x95, 0x06,       //   Report Count (6)
    0x75, 0x08,       //   Report Size (8)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x65,       //   Logical Maximum (101)
    0x05, 0x07,       //   Usage Page (Keyboard)
    0x19, 0x00,       //   Usage Minimum (0)
    0x29, 0x65,       //   Usage Maximum (101)
    0x81, 0x00,       //   Input (Data, Array)
    0xC0,             // End Collection

    // Mouse (可选)
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x02,       // Usage (Mouse)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x02,       //   Report ID (2)
    0x09, 0x01,       //   Usage (Pointer)
    0xA1, 0x00,       //   Collection (Physical)
    // ... mouse 描述符
    0xC0,             //   End Collection
    0xC0,             // End Collection

    // Consumer Control (媒体键)
    0x05, 0x0C,       // Usage Page (Consumer)
    0x09, 0x01,       // Usage (Consumer Control)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x03,       //   Report ID (3)
    // ... consumer 描述符
    0xC0,             // End Collection
};
```

### 3.4 多设备切换实现

```c
// 设备槽位管理
typedef struct {
    uint8_t mac_addr[6];      // 已绑定设备的MAC地址
    uint8_t ltk[16];          // Long Term Key
    uint8_t ediv[2];          // Encrypted Diversifier
    uint8_t rand[8];          // Random Number
    bool valid;               // 槽位是否有效
} ble_bond_info_t;

static ble_bond_info_t bond_info[BLE_SLOT_COUNT];
static uint8_t current_slot = 0;

// 切换设备槽位
void ble_switch_slot(uint8_t slot) {
    if (slot >= BLE_SLOT_COUNT) return;

    // 断开当前连接
    if (is_ble_connected()) {
        GAPRole_TerminateConnection();
    }

    // 切换槽位
    current_slot = slot;

    // 如果槽位有绑定信息，尝试定向广播
    if (bond_info[slot].valid) {
        // 定向广播到已绑定设备
        start_directed_advertising(&bond_info[slot]);
    } else {
        // 普通广播等待新配对
        start_general_advertising();
    }
}
```

## 4. 状态机设计

### 4.1 连接状态机

```
                    ┌─────────────────┐
                    │     IDLE        │
                    │  (未初始化)     │
                    └────────┬────────┘
                             │ init()
                             ▼
                    ┌─────────────────┐
                    │  ADVERTISING    │
                    │   (广播中)      │◄─────┐
                    └────────┬────────┘      │
                             │ connect       │ timeout/
                             │               │ disconnect
                             ▼               │
                    ┌─────────────────┐      │
                    │   CONNECTED     │──────┘
                    │   (已连接)      │
                    └────────┬────────┘
                             │ sleep
                             ▼
                    ┌─────────────────┐
                    │    SLEEPING     │
                    │   (睡眠中)      │
                    └─────────────────┘
```

### 4.2 配对状态机

```
                    ┌─────────────────┐
                    │   NOT_PAIRED    │
                    │   (未配对)      │
                    └────────┬────────┘
                             │ pairing_request
                             ▼
                    ┌─────────────────┐
                    │    PAIRING      │
                    │   (配对中)      │
                    └────────┬────────┘
                             │ success
                             ▼
                    ┌─────────────────┐
                    │    BONDED       │
                    │   (已绑定)      │
                    └─────────────────┘
```

## 5. 低功耗设计

### 5.1 功耗模式

| 模式 | 连接间隔 | 矩阵扫描 | RGB | 预计功耗 |
|------|----------|----------|-----|----------|
| 活动 | 7.5ms | 1000Hz | 开启 | ~15mA |
| 正常 | 15ms | 100Hz | 关闭 | ~3mA |
| 空闲 | 100ms | 10Hz | 关闭 | ~500uA |
| 睡眠 | 断开 | 中断唤醒 | 关闭 | ~10uA |

### 5.2 低功耗策略

```c
// 低功耗管理
typedef enum {
    POWER_MODE_ACTIVE,     // 正在输入
    POWER_MODE_IDLE,       // 短时间空闲
    POWER_MODE_SLEEP,      // 长时间空闲
} power_mode_t;

static uint32_t last_activity_time = 0;
#define IDLE_TIMEOUT_MS    30000   // 30秒进入空闲
#define SLEEP_TIMEOUT_MS   300000  // 5分钟进入睡眠

void power_management_task(void) {
    uint32_t elapsed = timer_elapsed32(last_activity_time);

    if (elapsed > SLEEP_TIMEOUT_MS) {
        enter_sleep_mode();
    } else if (elapsed > IDLE_TIMEOUT_MS) {
        enter_idle_mode();
    }
}

void on_keypress(void) {
    last_activity_time = timer_read32();
    if (current_power_mode != POWER_MODE_ACTIVE) {
        exit_low_power_mode();
    }
}
```

## 6. 实现优先级

1. **第一优先级**: 基本连接和按键发送
   - BLE初始化
   - HID服务注册
   - 键盘报告发送

2. **第二优先级**: 配对和绑定
   - 配对流程
   - 绑定信息存储
   - 自动重连

3. **第三优先级**: 多设备支持
   - 设备槽位管理
   - 槽位切换

4. **第四优先级**: 低功耗优化
   - 空闲检测
   - 睡眠模式
   - 唤醒机制

---

*文档创建日期: 2026-01-19*
