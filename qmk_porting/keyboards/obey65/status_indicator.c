/*
 * Status Indicator for Obey65
 * Phase 4.3 - LED status indication
 */

#include "status_indicator.h"
#include "wireless_mode.h"
#include "battery.h"
#include "timer.h"
#include "debug_uart.h"
#include "qmk_config.h"

#ifdef RGB_MATRIX_ENABLE
#include "rgb_matrix.h"
#endif

// Indicator LED indices (from config.h)
#ifndef BATTERY_INDICATOR_START_INDEX
#define BATTERY_INDICATOR_START_INDEX 3
#endif
#ifndef BATTERY_INDICATOR_END_INDEX
#define BATTERY_INDICATOR_END_INDEX 0
#endif

// Number of indicator LEDs
#define INDICATOR_LED_COUNT (BATTERY_INDICATOR_START_INDEX - BATTERY_INDICATOR_END_INDEX + 1)

// Timing constants
#define BLINK_INTERVAL_MS       500     // Blinking interval for connecting state
#define BATTERY_DISPLAY_MS      3000    // How long to show battery level
#define AUTO_SWITCH_INTERVAL_MS 5000    // Auto switch interval in AUTO mode

// Mode colors (RGB format: 0x00RRGGBB)
#define COLOR_USB_MODE      0x000000FF  // Blue
#define COLOR_BLE_MODE      0x0000FFFF  // Cyan
#define COLOR_ESB_MODE      0x0000FF00  // Green
#define COLOR_ERROR         0x00FF0000  // Red
#define COLOR_OFF           0x00000000  // Off

// Battery level colors
#define COLOR_BAT_FULL      0x0000FF00  // Green (76-100%)
#define COLOR_BAT_GOOD      0x0080FF00  // Yellow-Green (51-75%)
#define COLOR_BAT_LOW       0x00FFFF00  // Yellow (26-50%)
#define COLOR_BAT_CRITICAL  0x00FF0000  // Red (0-25%)
#define COLOR_BAT_CHARGING  0x00FF8000  // Orange

// State
static struct {
    indicator_mode_t mode;
    uint32_t last_blink_time;
    bool blink_state;
    uint32_t battery_display_until;
    uint32_t last_auto_switch;
    bool showing_battery;
} ind_state = {
    .mode = INDICATOR_MODE_AUTO,
    .last_blink_time = 0,
    .blink_state = false,
    .battery_display_until = 0,
    .last_auto_switch = 0,
    .showing_battery = false
};

// Internal functions
static uint32_t get_mode_color(void);
static void update_indicator_leds(void);
static void set_indicator_led(uint8_t index, uint32_t color);

void status_indicator_init(void) {
    ind_state.mode = INDICATOR_MODE_AUTO;
    ind_state.last_blink_time = timer_read32();
    ind_state.blink_state = true;
    ind_state.battery_display_until = 0;
    ind_state.last_auto_switch = timer_read32();
    ind_state.showing_battery = false;

    DEBUG_PRINTF("[IND] Status indicator initialized\r\n");
}

void status_indicator_task(void) {
    // Update blink state
    if (timer_elapsed32(ind_state.last_blink_time) >= BLINK_INTERVAL_MS) {
        ind_state.last_blink_time = timer_read32();
        ind_state.blink_state = !ind_state.blink_state;
    }

    // Handle AUTO mode switching
    if (ind_state.mode == INDICATOR_MODE_AUTO) {
        // Check if battery display time expired
        if (ind_state.battery_display_until > 0 &&
            timer_read32() >= ind_state.battery_display_until) {
            ind_state.battery_display_until = 0;
            ind_state.showing_battery = false;
        }

        // Auto-switch between status and battery
        if (timer_elapsed32(ind_state.last_auto_switch) >= AUTO_SWITCH_INTERVAL_MS) {
            ind_state.last_auto_switch = timer_read32();
            ind_state.showing_battery = !ind_state.showing_battery;
        }
    }

    // Update LED colors
    update_indicator_leds();
}

void status_indicator_set_mode(indicator_mode_t mode) {
    ind_state.mode = mode;
    DEBUG_PRINTF("[IND] Mode: %d\r\n", mode);
}

indicator_mode_t status_indicator_get_mode(void) {
    return ind_state.mode;
}

void status_indicator_show_battery(uint16_t duration_ms) {
    ind_state.battery_display_until = timer_read32() + duration_ms;
    ind_state.showing_battery = true;
}

uint32_t status_indicator_get_mode_color(void) {
    return get_mode_color();
}

bool status_indicator_is_blinking(void) {
    mode_status_t status = wireless_mode_get_status();
    return (status == MODE_STATUS_CONNECTING);
}

// Internal: Get color based on current wireless mode
static uint32_t get_mode_color(void) {
    wireless_mode_t mode = wireless_mode_get();
    mode_status_t status = wireless_mode_get_status();

    uint32_t color = COLOR_OFF;

    switch (mode) {
        case WIRELESS_MODE_USB:
            color = COLOR_USB_MODE;
            break;
        case WIRELESS_MODE_BLE:
            color = COLOR_BLE_MODE;
            break;
        case WIRELESS_MODE_ESB:
            color = COLOR_ESB_MODE;
            break;
        default:
            color = COLOR_ERROR;
            break;
    }

    // Handle connection status
    switch (status) {
        case MODE_STATUS_CONNECTED:
            // Full brightness
            break;
        case MODE_STATUS_CONNECTING:
            // Blink
            if (!ind_state.blink_state) {
                color = COLOR_OFF;
            }
            break;
        case MODE_STATUS_DISCONNECTED:
            // Dim (25% brightness)
            color = ((color >> 2) & 0x003F3F3F);
            break;
        case MODE_STATUS_ERROR:
            color = COLOR_ERROR;
            break;
    }

    return color;
}

// Internal: Get battery indicator colors
static void get_battery_colors(uint32_t *colors) {
    uint8_t percent = obey65_battery_get_percent();
    bool charging = obey65_battery_is_charging();

    // Calculate how many LEDs to light
    // 4 LEDs: each represents 25%
    uint8_t lit_count = (percent + 24) / 25;  // Round up
    if (lit_count > INDICATOR_LED_COUNT) lit_count = INDICATOR_LED_COUNT;

    // Determine color based on level
    uint32_t level_color;
    if (charging) {
        level_color = COLOR_BAT_CHARGING;
    } else if (percent > 75) {
        level_color = COLOR_BAT_FULL;
    } else if (percent > 50) {
        level_color = COLOR_BAT_GOOD;
    } else if (percent > 25) {
        level_color = COLOR_BAT_LOW;
    } else {
        level_color = COLOR_BAT_CRITICAL;
        // Critical level: blink the last LED
        if (percent <= 10 && !ind_state.blink_state) {
            lit_count = 0;
        }
    }

    // Set colors for each LED
    for (uint8_t i = 0; i < INDICATOR_LED_COUNT; i++) {
        if (i < lit_count) {
            colors[i] = level_color;
        } else {
            colors[i] = COLOR_OFF;
        }
    }
}

// Internal: Update indicator LEDs
static void update_indicator_leds(void) {
    uint32_t colors[INDICATOR_LED_COUNT];

    if (ind_state.mode == INDICATOR_MODE_OFF) {
        // All off
        for (uint8_t i = 0; i < INDICATOR_LED_COUNT; i++) {
            colors[i] = COLOR_OFF;
        }
    } else if (ind_state.mode == INDICATOR_MODE_BATTERY ||
               (ind_state.mode == INDICATOR_MODE_AUTO && ind_state.showing_battery)) {
        // Battery display
        get_battery_colors(colors);
    } else {
        // Mode/status display - all LEDs same color
        uint32_t mode_color = get_mode_color();
        for (uint8_t i = 0; i < INDICATOR_LED_COUNT; i++) {
            colors[i] = mode_color;
        }
    }

    // Apply colors to LEDs
    for (uint8_t i = 0; i < INDICATOR_LED_COUNT; i++) {
        // LED indices are reversed: START_INDEX is first LED, END_INDEX is last
        uint8_t led_index = BATTERY_INDICATOR_START_INDEX - i;
        set_indicator_led(led_index, colors[i]);
    }
}

// Internal: Set a single indicator LED color
static void set_indicator_led(uint8_t index, uint32_t color) {
#ifdef RGB_MATRIX_ENABLE
    // Extract RGB components from 0x00RRGGBB format
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    // Scale by maximum brightness
    r = (r * RGB_MATRIX_MAXIMUM_BRIGHTNESS) / 255;
    g = (g * RGB_MATRIX_MAXIMUM_BRIGHTNESS) / 255;
    b = (b * RGB_MATRIX_MAXIMUM_BRIGHTNESS) / 255;

    rgb_matrix_set_color(index, r, g, b);
#else
    (void)index;
    (void)color;
#endif
}
