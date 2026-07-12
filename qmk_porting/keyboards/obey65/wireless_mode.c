/*
 * Wireless Mode Management for Obey65
 * Handles USB/BLE/2.4G mode switching
 */

#include "wireless_mode.h"
#include "debug_uart.h"
#include "battery.h"
#include "timer.h"
#include "quantum_keycodes.h"  // For QK_KB_0
#include "status_indicator.h"  // For battery display
#include "bootloader.h"        // For EEPROM boot mode + mcu_reset()

#ifdef BLE_ENABLE
#include "protocol_ble.h"
#endif

#ifdef ESB_ENABLE
#include "protocol_esb.h"
#endif

// USB auto-detection debounce time (ms)
#define USB_DETECT_DEBOUNCE_MS 500

// Current state
static struct {
    wireless_mode_t current_mode;
    wireless_mode_t target_mode;
    wireless_mode_t previous_mode;  // Mode before USB auto-switch
    mode_status_t status;
    ble_slot_t ble_slot;
    bool mode_change_pending;
    uint32_t mode_change_timestamp;
    mode_change_callback_t callback;
    // USB auto-detection
    bool usb_auto_enabled;
    bool usb_was_connected;
    uint32_t usb_detect_timestamp;
} wm_state = {
    .current_mode = WIRELESS_MODE_USB,
    .target_mode = WIRELESS_MODE_USB,
    .previous_mode = WIRELESS_MODE_USB,
    .status = MODE_STATUS_DISCONNECTED,
    .ble_slot = BLE_SLOT_0,
    .mode_change_pending = false,
    .mode_change_timestamp = 0,
    .callback = NULL,
    .usb_auto_enabled = true,  // Enabled by default
    .usb_was_connected = false,
    .usb_detect_timestamp = 0
};

// Mode names for debug
static const char *mode_names[] = {
    [WIRELESS_MODE_USB] = "USB",
    [WIRELESS_MODE_BLE] = "BLE",
    [WIRELESS_MODE_ESB] = "ESB"
};

static const char *status_names[] = {
    [MODE_STATUS_DISCONNECTED] = "Disconnected",
    [MODE_STATUS_CONNECTING] = "Connecting",
    [MODE_STATUS_CONNECTED] = "Connected",
    [MODE_STATUS_ERROR] = "Error"
};

// Internal functions
static void apply_mode_change(void);
static void exit_current_mode(void);
static void enter_mode(wireless_mode_t mode);
static void check_usb_auto_switch(void);

void wireless_mode_init(void) {
    // Read actual boot mode from EEPROM so the indicator matches the running protocol.
    wireless_mode_t boot_mode = WIRELESS_MODE_USB;
    uint8_t eeprom_mode = bootloader_boot_mode_get();
    if (eeprom_mode == BOOTLOADER_BOOT_MODE_BLE) {
        boot_mode = WIRELESS_MODE_BLE;
    } else if (eeprom_mode == BOOTLOADER_BOOT_MODE_ESB) {
        boot_mode = WIRELESS_MODE_ESB;
    }

    wm_state.current_mode = boot_mode;
    wm_state.target_mode = boot_mode;
    wm_state.previous_mode = boot_mode;
    wm_state.status = MODE_STATUS_CONNECTING;
    wm_state.ble_slot = BLE_SLOT_0;
    wm_state.mode_change_pending = false;
    wm_state.callback = NULL;

    // No POWER_DETECT_PIN: disable USB auto-switch to avoid spurious mode changes.
    wm_state.usb_auto_enabled = false;
    wm_state.usb_was_connected = false;
    wm_state.usb_detect_timestamp = timer_read32();

    DEBUG_PRINTF("[MODE] Init: %s (EEPROM=0x%02X)\r\n",
                 wireless_mode_name(boot_mode), eeprom_mode);
}

wireless_mode_t wireless_mode_get(void) {
    return wm_state.current_mode;
}

mode_status_t wireless_mode_get_status(void) {
    return wm_state.status;
}

bool wireless_mode_switch(wireless_mode_t mode) {
    if (mode >= WIRELESS_MODE_COUNT) {
        DEBUG_PRINTF("[MODE] Invalid mode: %d\r\n", mode);
        return false;
    }

    if (mode == wm_state.current_mode && !wm_state.mode_change_pending) {
        DEBUG_PRINTF("[MODE] Already in %s mode\r\n", wireless_mode_name(mode));
        return true;
    }

    if (!wireless_mode_available(mode)) {
        DEBUG_PRINTF("[MODE] Mode %s not available\r\n", wireless_mode_name(mode));
        return false;
    }

    DEBUG_PRINTF("[MODE] Switch: %s -> %s\r\n",
                 wireless_mode_name(wm_state.current_mode),
                 wireless_mode_name(mode));

    wm_state.target_mode = mode;
    wm_state.mode_change_pending = true;
    wm_state.mode_change_timestamp = 0; // timer_read32();

    return true;
}

bool wireless_mode_switch_ble_slot(ble_slot_t slot) {
    if (slot >= BLE_SLOT_MAX) {
        return false;
    }

#ifndef BLE_ENABLE
    return false;
#else
    if (!ble_slot_is_supported(slot)) {
        DEBUG_PRINTF("[MODE] BLE slot %d is not supported\r\n", slot);
        return false;
    }

    if (wm_state.current_mode == WIRELESS_MODE_BLE && !ble_switch_slot(slot)) {
        return false;
    }

    DEBUG_PRINTF("[MODE] BLE slot: %d\r\n", slot);
    wm_state.ble_slot = slot;
    return true;
#endif
}

ble_slot_t wireless_mode_get_ble_slot(void) {
    return wm_state.ble_slot;
}

bool wireless_mode_available(wireless_mode_t mode) {
    switch (mode) {
        case WIRELESS_MODE_USB:
#ifdef USB_ENABLE
            return true;
#else
            return false;
#endif

        case WIRELESS_MODE_BLE:
#ifdef BLE_ENABLE
            return true;
#else
            return false;
#endif

        case WIRELESS_MODE_ESB:
#ifdef ESB_ENABLE
            return true;
#else
            return false;
#endif

        default:
            return false;
    }
}

void wireless_mode_task(void) {
    // Check USB auto-switch
    check_usb_auto_switch();

    // Process pending mode change
    if (wm_state.mode_change_pending) {
        apply_mode_change();
    }

    // Update status based on current mode
    switch (wm_state.current_mode) {
        case WIRELESS_MODE_USB:
            // TODO: Check USB connection status
            // wm_state.status = usb_connected() ? MODE_STATUS_CONNECTED : MODE_STATUS_DISCONNECTED;
            wm_state.status = MODE_STATUS_CONNECTED; // Assume connected for now
            break;

        case WIRELESS_MODE_BLE:
#ifdef BLE_ENABLE
            wm_state.status = ble_is_connected() ? MODE_STATUS_CONNECTED : MODE_STATUS_CONNECTING;
#endif
            break;

        case WIRELESS_MODE_ESB:
#ifdef ESB_ENABLE
            // TODO: wm_state.status = esb_get_connection_status();
#endif
            break;

        default:
            break;
    }
}

void wireless_mode_register_callback(mode_change_callback_t callback) {
    wm_state.callback = callback;
}

static void apply_mode_change(void) {
    if (!wm_state.mode_change_pending) {
        return;
    }

    wireless_mode_t old_mode = wm_state.current_mode;
    wireless_mode_t new_mode = wm_state.target_mode;

    // Exit current mode
    exit_current_mode();

    // Enter new mode
    enter_mode(new_mode);

    // Update state
    wm_state.current_mode = new_mode;
    wm_state.mode_change_pending = false;
    wm_state.status = MODE_STATUS_CONNECTING;

    // Notify callback
    if (wm_state.callback) {
        wm_state.callback(old_mode, new_mode);
    }

    DEBUG_PRINTF("[MODE] Changed: %s -> %s\r\n",
                 wireless_mode_name(old_mode),
                 wireless_mode_name(new_mode));

    // TODO: Save to EEPROM
    // eeprom_write_wireless_mode(new_mode);
}

static void exit_current_mode(void) {
    switch (wm_state.current_mode) {
        case WIRELESS_MODE_USB:
            // USB doesn't need cleanup
            break;

        case WIRELESS_MODE_BLE:
#ifdef BLE_ENABLE
            ble_disconnect();
            ble_stop_advertising();
#endif
            break;

        case WIRELESS_MODE_ESB:
#ifdef ESB_ENABLE
            esb_disconnect();
#endif
            break;

        default:
            break;
    }
}

static void enter_mode(wireless_mode_t mode) {
    // BLE/ESB protocol stacks are only initialized when the MCU boots in that
    // mode (via platform_initialize). A runtime switch requires writing the
    // desired mode to EEPROM and rebooting so the correct stack initializes.
    switch (mode) {
        case WIRELESS_MODE_USB:
            DEBUG_PRINTF("[MODE] -> USB, rebooting\r\n");
            bootloader_boot_mode_set(BOOTLOADER_BOOT_MODE_USB);
            mcu_reset();
            break;

        case WIRELESS_MODE_BLE:
#ifdef BLE_ENABLE
            DEBUG_PRINTF("[MODE] -> BLE slot %d, rebooting\r\n", wm_state.ble_slot);
            bootloader_boot_mode_set(BOOTLOADER_BOOT_MODE_BLE);
            mcu_reset();
#endif
            break;

        case WIRELESS_MODE_ESB:
#ifdef ESB_ENABLE
            DEBUG_PRINTF("[MODE] -> ESB, rebooting\r\n");
            bootloader_boot_mode_set(BOOTLOADER_BOOT_MODE_ESB);
            mcu_reset();
#endif
            break;

        default:
            break;
    }
}

bool process_wireless_keycode(uint16_t keycode, bool pressed) {
    // Only process on key press
    if (!pressed) {
        return false;
    }

    if (keycode == WL_USB) {
        if (wireless_mode_switch(WIRELESS_MODE_USB)) {
            wm_state.previous_mode = WIRELESS_MODE_USB;
        }
        return true;
    }

    if (keycode == WL_ESB) {
        if (wireless_mode_switch(WIRELESS_MODE_ESB)) {
            wm_state.previous_mode = WIRELESS_MODE_ESB;
        }
        return true;
    }

    if (keycode == WL_BLE0) {
        if (wireless_mode_switch_ble_slot(BLE_SLOT_0)) {
            wm_state.previous_mode = WIRELESS_MODE_BLE;  // Remember as explicit choice
            (void)wireless_mode_switch(WIRELESS_MODE_BLE);
        }
        return true;
    }

    // Keep legacy BLE2..BLE16 keycodes inert instead of reassigning their ABI.
    if (keycode >= (QK_KB_0 + 13) && keycode <= (QK_KB_0 + 27)) {
        return true;
    }

    if (keycode == WL_BATTERY) {
#ifdef RGB_MATRIX_ENABLE
        status_indicator_show_battery(3000);  // Show battery for 3 seconds
#endif
        return true;
    }

    return false;
}

uint32_t wireless_mode_get_indicator_color(void) {
    // Colors for mode indication
    // USB: Blue, BLE: Cyan, ESB: Green
    // Disconnected: dim, Connecting: blink, Connected: solid

    uint32_t base_color = 0;

    switch (wm_state.current_mode) {
        case WIRELESS_MODE_USB:
            base_color = 0x000000FF; // Blue
            break;
        case WIRELESS_MODE_BLE:
            base_color = 0x0000FFFF; // Cyan
            break;
        case WIRELESS_MODE_ESB:
            base_color = 0x0000FF00; // Green
            break;
        default:
            base_color = 0x00FF0000; // Red for error
            break;
    }

    // Dim the color if disconnected
    if (wm_state.status == MODE_STATUS_DISCONNECTED) {
        base_color = (base_color >> 2) & 0x003F3F3F;
    }

    return base_color;
}

const char *wireless_mode_name(wireless_mode_t mode) {
    if (mode < WIRELESS_MODE_COUNT) {
        return mode_names[mode];
    }
    return "Unknown";
}

const char *wireless_status_name(mode_status_t status) {
    if (status <= MODE_STATUS_ERROR) {
        return status_names[status];
    }
    return "Unknown";
}

void wireless_mode_set_usb_auto(bool enable) {
    wm_state.usb_auto_enabled = enable;
    DEBUG_PRINTF("[MODE] USB auto-switch: %s\r\n", enable ? "enabled" : "disabled");
}

bool wireless_mode_get_usb_auto(void) {
    return wm_state.usb_auto_enabled;
}

static void check_usb_auto_switch(void) {
    if (!wm_state.usb_auto_enabled) {
        return;
    }

    bool usb_connected = obey65_battery_is_usb_connected();

    // Debounce USB detection
    if (usb_connected != wm_state.usb_was_connected) {
        if (timer_elapsed32(wm_state.usb_detect_timestamp) < USB_DETECT_DEBOUNCE_MS) {
            return;  // Still within debounce period
        }

        wm_state.usb_detect_timestamp = timer_read32();

        if (usb_connected && !wm_state.usb_was_connected) {
            // USB plugged in - switch to USB mode
            DEBUG_PRINTF("[MODE] USB plugged in, auto-switching to USB\r\n");
            wm_state.previous_mode = wm_state.current_mode;
            wireless_mode_switch(WIRELESS_MODE_USB);
        } else if (!usb_connected && wm_state.usb_was_connected) {
            // USB unplugged - return to previous wireless mode
            DEBUG_PRINTF("[MODE] USB unplugged, returning to %s\r\n",
                         wireless_mode_name(wm_state.previous_mode));
            if (wm_state.previous_mode != WIRELESS_MODE_USB) {
                wireless_mode_switch(wm_state.previous_mode);
            } else {
                // Previous was also USB, try BLE first, then ESB
#ifdef BLE_ENABLE
                wireless_mode_switch(WIRELESS_MODE_BLE);
#elif defined(ESB_ENABLE)
                wireless_mode_switch(WIRELESS_MODE_ESB);
#endif
            }
        }

        wm_state.usb_was_connected = usb_connected;
    } else {
        // Reset debounce timestamp when state is stable
        wm_state.usb_detect_timestamp = timer_read32();
    }
}
