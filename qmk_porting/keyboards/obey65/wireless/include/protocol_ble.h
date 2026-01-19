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
