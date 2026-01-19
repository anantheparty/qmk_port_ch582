/*
 * Battery interface for Obey65
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Initialize battery measurement (called by platform battery_init)
// No additional init needed as platform handles ADC setup

// Update battery status (call periodically, e.g., every 5 seconds)
void obey65_battery_update(void);

// Get battery percentage (0-100)
uint8_t obey65_battery_get_percent(void);

// Get battery voltage in millivolts
uint16_t obey65_battery_get_voltage(void);

// Check if battery is charging
bool obey65_battery_is_charging(void);

// Check if USB is connected
bool obey65_battery_is_usb_connected(void);

// Platform callback - required by battery_measure.c
void battery_critical_gpio_prerequisite(void);
