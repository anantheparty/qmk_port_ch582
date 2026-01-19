#pragma once

#include "protocol.h"
#include <stdbool.h>

// BLE 协议接口
extern const ch582_interface_t ch582_protocol_ble;

// BLE 状态查询
bool ble_is_connected(void);

// BLE 控制
void ble_start_advertising(void);
void ble_stop_advertising(void);
void ble_disconnect(void);
