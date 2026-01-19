# Phase 2: 蓝牙开发总结

**完成日期**: 2026-01-19
**开发周期**: 1 天
**提交数量**: 6 个

## 概述

Phase 2 成功实现了 Obey65 键盘的蓝牙 HID 功能，包括 BLE 5.3 协议栈集成、HID over GATT 服务、多设备配对管理和功耗优化。

## 固件状态

```
Memory region         Used Size  Region Size  %age Used
       FLASH:      231612 B       372 KB     60.80%
         RAM:       28340 B        32 KB     86.49%
```

相比 Phase 1 (105756B FLASH, 24160B RAM)，BLE 功能增加约 126KB FLASH 和 4KB RAM。

---

## 主要成果

### Phase 2.0: BLE 编译修复

**关键阻塞问题解决**

**问题描述**: 启用 `BLE_ENABLE=ON` 时，链接器报错找不到 `_start` 符号。

**根本原因分析**:
1. `sdk/CMakeLists.txt` 第 56 行的条件逻辑存在问题
2. 当 BLE 启用时，启动代码 `startup_CH583.S` 不会被包含到编译目标
3. 条件判断中未考虑 `BUILD_WIRELESS_FROM_SOURCE` 变量

**解决方案**:

```cmake
# sdk/CMakeLists.txt 修改
# 原代码只在 BLE/ESB 禁用时添加启动代码
# 修改为：在 BUILD_WIRELESS_FROM_SOURCE 模式下也添加启动代码

if(NOT (BLE_ENABLE OR ESB_ENABLE) OR BUILD_WIRELESS_FROM_SOURCE)
    # 添加启动代码
endif()
```

**创建 `ble_support.c`**:
```c
// 提供 BLE 协议栈所需的变量定义
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];
uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};

// 无线库接口桩函数
void RF_LibInit(uint32_t tick, uint8_t *addr) { ... }
```

**commit**: e646b2b4

---

### Phase 2.1: BLE 初始化与广播

**实现内容**:
- GAP Peripheral Role 配置
- 状态变化回调 (`ble_StateNotificationCB`)
- 配对状态回调 (`ble_PairStateCB`)
- 密码回调 (`ble_PasscodeCB`) - 自动接受配对
- 广播数据配置 (设备名称、外观、服务 UUID)
- 低延迟连接参数 (7.5ms-15ms)

**关键代码**:
```c
// protocol_ble.c
static uint8_t advertData[] = {
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
    GAP_APPEARE_HID_KEYBOARD,
    HID_SERV_UUID
};

GAPRole_PeripheralStartDevice(bleTaskId, &ble_BondMgrCBs, &ble_PeripheralCBs);
```

**commit**: 3ec100e0

---

### Phase 2.2: HID over GATT 服务

**实现内容**:
- 完善 HID Report Map 描述符
  - Keyboard Report (Report ID: 1)
  - Mouse Report (Report ID: 2)
  - Consumer Control Report (Report ID: 3)
  - System Control Report (Report ID: 4)
- 键盘报告发送 (`send_keyboard`)
- LED 状态接收 (`HidDev_GetKeyboardLeds`)
- Consumer Control 支持 (媒体键)
- System Control 支持 (电源/睡眠)
- Boot Protocol 属性

**HID Report Map 结构**:
```c
static const uint8_t hidReportMap[] = {
    // Keyboard (ID: 1) - 8 bytes
    0x05, 0x01,  // Usage Page (Generic Desktop)
    0x09, 0x06,  // Usage (Keyboard)
    ...
    // Consumer Control (ID: 3) - 2 bytes
    0x05, 0x0C,  // Usage Page (Consumer)
    0x09, 0x01,  // Usage (Consumer Control)
    ...
    // System Control (ID: 4) - 1 byte
    0x05, 0x01,  // Usage Page (Generic Desktop)
    0x09, 0x80,  // Usage (System Control)
    ...
};
```

**commit**: a861c6b7

---

### Phase 2.3: 配对管理

**实现内容**:
- 配对流程 (GAPBondMgr 集成)
- 绑定信息存储 (`ble_bonding.c`)
- 配对槽位切换 (`ble_switch_slot`)
- 清除所有配对 (`ble_clear_all_bonds`)
- 4 个配对槽位支持 (`BLE_MAX_BONDS=4`)

**关键数据结构**:
```c
typedef enum {
    BOND_SLOT_EMPTY = 0,
    BOND_SLOT_BONDED,
    BOND_SLOT_ACTIVE
} bond_slot_state_t;

typedef struct {
    bond_slot_state_t state;
    uint8_t addr[6];
    uint8_t addr_type;
} bond_slot_info_t;
```

**API**:
```c
void ble_bonding_init(void);
bool ble_bonding_set_active_slot(uint8_t slot);
uint8_t ble_bonding_get_active_slot(void);
void ble_bonding_clear_all(void);
uint8_t ble_bonding_get_count(void);
```

**commit**: a71b47b9

---

### Phase 2.4: 多设备连接

**实现内容**:
- `wireless_mode` 模块与 BLE 集成
- 模式切换时正确调用 BLE 函数
- 设备切换逻辑完善

**集成点**:
```c
// wireless_mode.c
static void enter_mode(wireless_mode_t mode) {
    switch (mode) {
        case WIRELESS_MODE_BLE:
#ifdef BLE_ENABLE
            ble_switch_slot(wm_state.ble_slot);
#endif
            break;
        ...
    }
}
```

**commit**: c327f177

---

### Phase 2.5: 蓝牙功耗优化

**实现内容**:
- BLE 连接状态感知的睡眠控制
- 动态连接参数调整
- 电源管理与 BLE 功耗模式集成

**功耗模式**:
```c
typedef enum {
    BLE_POWER_LOW_LATENCY = 0,  // 7.5-15ms 连接间隔 (打字时)
    BLE_POWER_SAVE              // 30-50ms 连接间隔 (空闲时)
} ble_power_mode_t;
```

**睡眠控制**:
```c
// power_mode.c
bool power_mode_sleep_blocked(void) {
#ifdef BLE_ENABLE
    if (wireless_mode_get() == WIRELESS_MODE_BLE && ble_is_connected()) {
        return true;  // BLE 连接时阻止深度睡眠
    }
#endif
    return pm_state.auto_sleep_disabled;
}
```

**睡眠参数**:
```c
// 睡眠时保持 BLE 单元供电以便快速重连
if (wireless_mode_get() == WIRELESS_MODE_BLE) {
    sleep_params |= RB_PWR_EXTEND;
}
LowPower_Sleep(sleep_params);
```

**commit**: fe3fb195

---

## 新增文件

| 文件 | 用途 |
|------|------|
| `wireless/src/protocol_ble.c` | BLE HID 协议实现 |
| `wireless/include/protocol_ble.h` | BLE 协议接口 |
| `wireless/src/hid_dev.c` | HID GATT 服务 |
| `wireless/src/hid_dev.h` | HID 服务头文件 |
| `wireless/src/ble_bonding.c` | 配对槽位管理 |
| `wireless/include/ble_bonding.h` | 配对管理接口 |
| `wireless/src/ble_support.c` | BLE 支持桩函数 |
| `wireless/include/ble_compat.h` | BLE SDK 兼容层 |

---

## 架构概述

```
+-------------------+     +------------------+
|  wireless_mode.c  |---->|  protocol_ble.c  |
+-------------------+     +------------------+
         |                        |
         v                        v
+-------------------+     +------------------+
|   power_mode.c    |     |    hid_dev.c     |
+-------------------+     +------------------+
                                  |
                                  v
                          +------------------+
                          |  ble_bonding.c   |
                          +------------------+
                                  |
                                  v
                          +------------------+
                          |  CH58xBLE_LIB    |
                          +------------------+
```

---

## 已知限制

1. **NKRO 支持**: BLE HID 通常不支持完整的 NKRO，可能需要拆分为多个报告
2. **连接参数**: 中央设备可能拒绝连接参数更新请求
3. **深度睡眠**: BLE 模式下不支持深度睡眠，以保持重连能力
4. **单连接**: 当前实现仅支持单设备连接

---

## 待优化项

1. 实际硬件测试验证
2. 连接稳定性优化
3. 重连机制改进
4. 电池电量 BLE 通知

---

## 下一步: Phase 3

Phase 3 将实现 2.4G ESB 无线功能：
- ESB 协议实现
- 接收器固件
- 2.4G 配对流程
- 2.4G 功耗优化

---

*文档生成时间: 2026-01-19*
