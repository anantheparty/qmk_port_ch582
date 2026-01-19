#pragma once

#include "protocol.h"
#include <stdbool.h>
#include <stdint.h>

// BLE 协议接口
extern const ch582_interface_t ch582_protocol_ble;

// BLE 状态查询
bool ble_is_connected(void);

// BLE 控制
void ble_start_advertising(void);
void ble_stop_advertising(void);
void ble_disconnect(void);

// BLE 配对管理
bool ble_switch_slot(uint8_t slot);
uint8_t ble_get_current_slot(void);
void ble_clear_all_bonds(void);
uint8_t ble_get_bond_count(void);

// BLE 功耗模式
typedef enum {
    BLE_POWER_LOW_LATENCY = 0,  // 低延迟模式 (7.5-15ms) - 打字时使用
    BLE_POWER_SAVE              // 省电模式 (30-50ms) - 空闲时使用
} ble_power_mode_t;

// BLE 功耗管理
void ble_set_power_mode(ble_power_mode_t mode);
ble_power_mode_t ble_get_power_mode(void);

// BLE 活动通知 (用于自动调整功耗模式)
void ble_on_key_activity(void);  // 有按键活动时调用
void ble_on_idle(void);          // 进入空闲状态时调用
