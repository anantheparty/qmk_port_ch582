/*
 * Status Indicator for Obey65
 * Phase 4.3 - LED status indication
 *
 * Uses RGB LEDs 0-3 (BATTERY_INDICATOR indices) for status display:
 * - Mode indication (USB/BLE/ESB)
 * - Connection status (solid/blinking)
 * - Battery level
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Status indicator mode
typedef enum {
    INDICATOR_MODE_OFF = 0,     // Indicator disabled
    INDICATOR_MODE_STATUS,      // Show wireless mode and connection status
    INDICATOR_MODE_BATTERY,     // Show battery level
    INDICATOR_MODE_AUTO         // Auto-switch between status and battery
} indicator_mode_t;

// Initialize status indicator
void status_indicator_init(void);

// Task function - call from housekeeping
void status_indicator_task(void);

// Set indicator mode
void status_indicator_set_mode(indicator_mode_t mode);
indicator_mode_t status_indicator_get_mode(void);

// Force battery display for a short time (e.g., on battery level change)
void status_indicator_show_battery(uint16_t duration_ms);

// Get color for mode indication
// This can be used by RGB matrix effects
uint32_t status_indicator_get_mode_color(void);

// Check if indicator should be blinking (connecting state)
bool status_indicator_is_blinking(void);
