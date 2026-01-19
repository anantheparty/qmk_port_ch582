/*
 * Wireless Mode Management for Obey65
 * Handles USB/BLE/2.4G mode switching
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Connection modes
typedef enum {
    WIRELESS_MODE_USB = 0,      // Wired USB mode
    WIRELESS_MODE_BLE,          // Bluetooth mode (Phase 2)
    WIRELESS_MODE_ESB,          // 2.4G ESB mode (Phase 3)
    WIRELESS_MODE_COUNT
} wireless_mode_t;

// BLE slot definitions (for multi-device pairing)
typedef enum {
    BLE_SLOT_0 = 0,
    BLE_SLOT_1,
    BLE_SLOT_2,
    BLE_SLOT_3,
    BLE_SLOT_MAX
} ble_slot_t;

// Mode status
typedef enum {
    MODE_STATUS_DISCONNECTED = 0,
    MODE_STATUS_CONNECTING,
    MODE_STATUS_CONNECTED,
    MODE_STATUS_ERROR
} mode_status_t;

// Callback type for mode change notifications
typedef void (*mode_change_callback_t)(wireless_mode_t old_mode, wireless_mode_t new_mode);

// Initialize wireless mode management
void wireless_mode_init(void);

// Get current mode
wireless_mode_t wireless_mode_get(void);

// Get current mode status
mode_status_t wireless_mode_get_status(void);

// Request mode switch (may be delayed if current mode needs cleanup)
bool wireless_mode_switch(wireless_mode_t mode);

// Switch to specific BLE slot
bool wireless_mode_switch_ble_slot(ble_slot_t slot);

// Get current BLE slot (only valid in BLE mode)
ble_slot_t wireless_mode_get_ble_slot(void);

// Check if a mode is available (e.g., USB only available when connected)
bool wireless_mode_available(wireless_mode_t mode);

// Task function - call in main loop
void wireless_mode_task(void);

// Register mode change callback
void wireless_mode_register_callback(mode_change_callback_t callback);

// Keycode handlers (for keymap integration)
bool process_wireless_keycode(uint16_t keycode, bool pressed);

// Mode indicator - returns color for RGB indication
// Format: 0x00RRGGBB
uint32_t wireless_mode_get_indicator_color(void);

// Debug helpers
const char *wireless_mode_name(wireless_mode_t mode);
const char *wireless_status_name(mode_status_t status);
