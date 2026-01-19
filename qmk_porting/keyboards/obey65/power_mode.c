/*
 * Power management for Obey65
 * Phase 1.4 - Basic power mode framework
 * Phase 2.5 - BLE power optimization
 */

#include "power_mode.h"
#include "CH58x_common.h"
#include "timer.h"
#include "gpio.h"
#include "debug_uart.h"
#include "qmk_config.h"

#ifdef RGB_MATRIX_ENABLE
#include "rgb_led.h"
#include "rgb_matrix.h"
#endif

#ifdef BLE_ENABLE
#include "wireless_mode.h"
#include "protocol_ble.h"
#endif

#ifdef ESB_ENABLE
#include "wireless_mode.h"
#include "protocol_esb.h"
#endif

// Phase 4.2: Battery integration
#include "battery.h"

// Battery check interval
#define BATTERY_CHECK_INTERVAL_MS 1000

// Default timeouts (can be configured)
#define DEFAULT_NORMAL_TIMEOUT_MS     5000      // 5 seconds
#define DEFAULT_IDLE_TIMEOUT_MS       30000     // 30 seconds
#define DEFAULT_SLEEP_TIMEOUT_MS      300000    // 5 minutes
#define DEFAULT_DEEP_SLEEP_TIMEOUT_MS 1800000   // 30 minutes

// Power mode state
static struct {
    power_mode_t current_mode;
    power_mode_t forced_mode;
    bool force_mode_enabled;
    bool auto_sleep_disabled;
    uint32_t last_activity_time;
    uint32_t normal_timeout_ms;
    uint32_t idle_timeout_ms;
    uint32_t sleep_timeout_ms;
    uint32_t deep_sleep_timeout_ms;
    // Phase 4.2: Power source tracking
    bool usb_powered;
    bool charging;
    uint32_t last_battery_check;
} pm_state = {
    .current_mode = POWER_MODE_ACTIVE,
    .forced_mode = POWER_MODE_ACTIVE,
    .force_mode_enabled = false,
    .auto_sleep_disabled = false,
    .last_activity_time = 0,
    .normal_timeout_ms = DEFAULT_NORMAL_TIMEOUT_MS,
    .idle_timeout_ms = DEFAULT_IDLE_TIMEOUT_MS,
    .sleep_timeout_ms = DEFAULT_SLEEP_TIMEOUT_MS,
    .deep_sleep_timeout_ms = DEFAULT_DEEP_SLEEP_TIMEOUT_MS,
    .usb_powered = false,
    .charging = false,
    .last_battery_check = 0,
};

// Mode names for debugging
static const char* mode_names[] = {
    "ACTIVE",
    "NORMAL",
    "IDLE",
    "SLEEP",
    "DEEP_SLEEP"
};

// Forward declarations
static void apply_power_mode(power_mode_t mode);
static void configure_wakeup_sources(void);
static void enter_idle_mode(void);
static void enter_sleep_mode(void);

void power_mode_init(void) {
    pm_state.last_activity_time = timer_read32();
    pm_state.current_mode = POWER_MODE_ACTIVE;

    DEBUG_PRINTF("[PWR] Init, mode: %s\r\n", mode_names[pm_state.current_mode]);
}

void power_mode_task(void) {
    // Phase 4.2: Periodically check battery/charging status
    if (timer_elapsed32(pm_state.last_battery_check) >= BATTERY_CHECK_INTERVAL_MS) {
        pm_state.last_battery_check = timer_read32();
        obey65_battery_update();
        pm_state.usb_powered = obey65_battery_is_usb_connected();
        pm_state.charging = obey65_battery_is_charging();
    }

    // Skip if forced mode is enabled
    if (pm_state.force_mode_enabled) {
        if (pm_state.current_mode != pm_state.forced_mode) {
            apply_power_mode(pm_state.forced_mode);
        }
        return;
    }

    // Phase 4.2: Skip auto-sleep when USB powered or charging
    // This keeps the keyboard fully responsive when connected
    if (pm_state.usb_powered || pm_state.charging) {
        // Stay in active mode when USB powered
        if (pm_state.current_mode > POWER_MODE_NORMAL) {
            apply_power_mode(POWER_MODE_ACTIVE);
        }
        return;
    }

    // Skip auto-sleep if disabled
    if (pm_state.auto_sleep_disabled) {
        return;
    }

    uint32_t elapsed = timer_elapsed32(pm_state.last_activity_time);
    power_mode_t target_mode = POWER_MODE_ACTIVE;

    // Determine target mode based on inactivity time
    if (elapsed >= pm_state.deep_sleep_timeout_ms) {
        target_mode = POWER_MODE_DEEP_SLEEP;
    } else if (elapsed >= pm_state.sleep_timeout_ms) {
        target_mode = POWER_MODE_SLEEP;
    } else if (elapsed >= pm_state.idle_timeout_ms) {
        target_mode = POWER_MODE_IDLE;
    } else if (elapsed >= pm_state.normal_timeout_ms) {
        target_mode = POWER_MODE_NORMAL;
    }

    // Check if deep sleep is blocked (e.g., by BLE)
    if (target_mode == POWER_MODE_DEEP_SLEEP && power_mode_deep_sleep_blocked()) {
        target_mode = POWER_MODE_SLEEP;  // Fall back to regular sleep
    }

    // Check if sleep is blocked
    if (target_mode == POWER_MODE_SLEEP && power_mode_sleep_blocked()) {
        target_mode = POWER_MODE_IDLE;  // Fall back to idle
    }

    // Apply mode change if needed
    if (target_mode != pm_state.current_mode) {
        apply_power_mode(target_mode);
    }
}

void power_mode_on_activity(void) {
    pm_state.last_activity_time = timer_read32();

    // Wake up if in low power mode
    if (pm_state.current_mode > POWER_MODE_ACTIVE) {
        DEBUG_PRINTF("[PWR] Activity detected, waking up\r\n");
        apply_power_mode(POWER_MODE_ACTIVE);
    }

#ifdef BLE_ENABLE
    // Notify BLE of key activity (switch to low latency mode)
    if (wireless_mode_get() == WIRELESS_MODE_BLE) {
        ble_on_key_activity();
    }
#endif

#ifdef ESB_ENABLE
    // Notify ESB of key activity (exit low power mode)
    if (wireless_mode_get() == WIRELESS_MODE_ESB) {
        esb_exit_low_power();
    }
#endif
}

power_mode_t power_mode_get(void) {
    return pm_state.current_mode;
}

const char* power_mode_name(power_mode_t mode) {
    if (mode < POWER_MODE_COUNT) {
        return mode_names[mode];
    }
    return "UNKNOWN";
}

void power_mode_set_force(power_mode_t mode) {
    pm_state.force_mode_enabled = true;
    pm_state.forced_mode = mode;
    DEBUG_PRINTF("[PWR] Force mode: %s\r\n", mode_names[mode]);
}

void power_mode_disable_auto_sleep(bool disable) {
    pm_state.auto_sleep_disabled = disable;
    DEBUG_PRINTF("[PWR] Auto-sleep %s\r\n", disable ? "disabled" : "enabled");
}

void power_mode_set_idle_timeout(uint32_t ms) {
    pm_state.idle_timeout_ms = ms;
}

void power_mode_set_sleep_timeout(uint32_t ms) {
    pm_state.sleep_timeout_ms = ms;
}

bool power_mode_sleep_blocked(void) {
    // Sleep is blocked during USB activity
#ifdef USB_ENABLE
    // TODO: Check USB active status
#endif

    // Sleep is blocked during BLE connection (only deep sleep)
    // BLE can still use idle/light sleep modes
#ifdef BLE_ENABLE
    if (wireless_mode_get() == WIRELESS_MODE_BLE && ble_is_connected()) {
        // When BLE connected, block deep sleep but allow idle
        return true;
    }
#endif

    return pm_state.auto_sleep_disabled;
}

bool power_mode_deep_sleep_blocked(void) {
    // Deep sleep is more restrictive
#ifdef BLE_ENABLE
    // Block deep sleep if in BLE mode (even if disconnected, to allow reconnection)
    if (wireless_mode_get() == WIRELESS_MODE_BLE) {
        return true;
    }
#endif

    return power_mode_sleep_blocked();
}

void power_mode_wakeup(void) {
    pm_state.last_activity_time = timer_read32();
    if (pm_state.current_mode != POWER_MODE_ACTIVE) {
        apply_power_mode(POWER_MODE_ACTIVE);
    }
}

// Internal functions

static void apply_power_mode(power_mode_t mode) {
    if (mode == pm_state.current_mode) {
        return;
    }

    DEBUG_PRINTF("[PWR] Mode: %s -> %s\r\n",
                 mode_names[pm_state.current_mode],
                 mode_names[mode]);

    power_mode_t prev_mode = pm_state.current_mode;
    pm_state.current_mode = mode;

    switch (mode) {
        case POWER_MODE_ACTIVE:
            // Full performance mode
#ifdef RGB_MATRIX_ENABLE
            if (prev_mode >= POWER_MODE_IDLE) {
                rgb_matrix_enable_noeeprom();
            }
#endif
            // Restore normal peripheral clocks
            PWR_PeriphClkCfg(ENABLE, BIT_SLP_CLK_TMR0 | BIT_SLP_CLK_TMR1 |
                                     BIT_SLP_CLK_TMR2 | BIT_SLP_CLK_TMR3);
            break;

        case POWER_MODE_NORMAL:
            // Slightly reduced performance
            // RGB still on but potentially reduced effects
            break;

        case POWER_MODE_IDLE:
            // Low power idle - RGB off
#ifdef RGB_MATRIX_ENABLE
            rgb_matrix_disable_noeeprom();
#endif
            // Reduce unnecessary peripheral clocks
            PWR_PeriphClkCfg(DISABLE, BIT_SLP_CLK_TMR2 | BIT_SLP_CLK_TMR3);

#ifdef BLE_ENABLE
            // Switch BLE to power saving mode when idle
            if (wireless_mode_get() == WIRELESS_MODE_BLE) {
                ble_on_idle();
            }
#endif

#ifdef ESB_ENABLE
            // Switch ESB to low power mode when idle
            if (wireless_mode_get() == WIRELESS_MODE_ESB) {
                esb_enter_low_power();
            }
#endif
            break;

        case POWER_MODE_SLEEP:
            enter_sleep_mode();
            break;

        case POWER_MODE_DEEP_SLEEP:
            // For now, same as sleep
            // TODO: Implement full shutdown mode
            enter_sleep_mode();
            break;

        default:
            break;
    }
}

static void configure_wakeup_sources(void) {
    // Configure GPIO wake sources (matrix pins)
    // Note: This is a simplified version. Full implementation
    // would configure specific matrix row/col pins.

    PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Long_Delay);

#ifdef USB_ENABLE
    // USB can wake the device
    PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_USB_WAKE, Long_Delay);
#endif

#ifdef POWER_DETECT_PIN
    // Battery/charging detection can wake
    PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_BAT_WAKE, Long_Delay);
#endif
}

static void __attribute__((unused)) enter_idle_mode(void) {
    // Simple idle - CPU halts but peripherals keep running
    LowPower_Idle();
}

static void enter_sleep_mode(void) {
    DEBUG_PRINTF("[PWR] Entering sleep mode...\r\n");

    // Disable RGB to save power
#ifdef RGB_MATRIX_ENABLE
    rgbled_power_off();
#endif

    // Configure wake sources
    configure_wakeup_sources();

    // Save any state needed
    // TODO: Save EEPROM state if needed

    // Determine sleep parameters based on current wireless mode
    uint8_t sleep_params = RB_PWR_RAM2K | RB_PWR_RAM30K;

#ifdef BLE_ENABLE
    // If in BLE mode, keep BLE unit powered for fast reconnection
    if (wireless_mode_get() == WIRELESS_MODE_BLE) {
        sleep_params |= RB_PWR_EXTEND;  // Keep USB/BLE unit powered
        DEBUG_PRINTF("[PWR] BLE mode: keeping BLE unit powered\r\n");
    }
#endif

    // Enter sleep with RAM retention
    LowPower_Sleep(sleep_params);

    // --- Execution continues here after wakeup ---

    DEBUG_PRINTF("[PWR] Woke up from sleep\r\n");

    // Restore system state
    pm_state.current_mode = POWER_MODE_ACTIVE;
    pm_state.last_activity_time = timer_read32();

    // Re-enable peripherals
#ifdef RGB_MATRIX_ENABLE
    rgb_matrix_enable_noeeprom();
#endif

#ifdef BLE_ENABLE
    // After wakeup, restart BLE advertising if in BLE mode
    if (wireless_mode_get() == WIRELESS_MODE_BLE && !ble_is_connected()) {
        ble_start_advertising();
    }
#endif
}

// ============================================================================
// Phase 4.2: Unified Power Management Functions
// ============================================================================

void power_mode_set_charging(bool charging) {
    if (pm_state.charging != charging) {
        pm_state.charging = charging;
        DEBUG_PRINTF("[PWR] Charging: %s\r\n", charging ? "YES" : "NO");

        // If started charging, wake up from any sleep mode
        if (charging && pm_state.current_mode > POWER_MODE_ACTIVE) {
            power_mode_wakeup();
        }
    }
}

bool power_mode_is_charging(void) {
    return pm_state.charging;
}

bool power_mode_usb_powered(void) {
    return pm_state.usb_powered;
}

power_source_t power_mode_get_source(void) {
    if (pm_state.charging) {
        return POWER_SOURCE_USB_CHARGING;
    } else if (pm_state.usb_powered) {
        return POWER_SOURCE_USB;
    }
    return POWER_SOURCE_BATTERY;
}
