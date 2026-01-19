/*
 * Power management for Obey65
 * Phase 1.4 - Basic power mode framework
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Power mode definitions
typedef enum {
    POWER_MODE_ACTIVE = 0,    // Full performance, RGB on
    POWER_MODE_NORMAL,        // Normal operation, reduced scan rate
    POWER_MODE_IDLE,          // RGB off, low scan rate
    POWER_MODE_SLEEP,         // CPU sleep, wake on keypress
    POWER_MODE_DEEP_SLEEP,    // Deepest sleep, minimal retention
    POWER_MODE_COUNT
} power_mode_t;

// Initialize power management
void power_mode_init(void);

// Main power management task (call from housekeeping)
void power_mode_task(void);

// Register user activity (keypress, encoder, etc.)
void power_mode_on_activity(void);

// Get current power mode
power_mode_t power_mode_get(void);

// Get power mode name string
const char* power_mode_name(power_mode_t mode);

// Force a specific power mode (for testing/debugging)
void power_mode_set_force(power_mode_t mode);

// Disable auto-sleep (useful during USB enumeration)
void power_mode_disable_auto_sleep(bool disable);

// Configure timeouts (in milliseconds)
void power_mode_set_idle_timeout(uint32_t ms);
void power_mode_set_sleep_timeout(uint32_t ms);

// Check if sleep is currently blocked
bool power_mode_sleep_blocked(void);

// Check if deep sleep is blocked (e.g., by BLE mode)
bool power_mode_deep_sleep_blocked(void);

// Wake up from sleep mode
void power_mode_wakeup(void);

// Phase 4.2: Unified power management

// Set charging state (called by battery module)
void power_mode_set_charging(bool charging);

// Check if currently charging
bool power_mode_is_charging(void);

// Check if USB power is connected
bool power_mode_usb_powered(void);

// Get current power source
typedef enum {
    POWER_SOURCE_BATTERY = 0,
    POWER_SOURCE_USB,
    POWER_SOURCE_USB_CHARGING
} power_source_t;

power_source_t power_mode_get_source(void);
