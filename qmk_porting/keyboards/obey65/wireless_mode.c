/*
 * Wireless Mode Management for Obey65
 * Handles USB/BLE/2.4G mode switching
 */

#include "wireless_mode.h"
#include "debug_uart.h"

#ifdef BLE_ENABLE
#include "protocol_ble.h"
#endif

#ifdef ESB_ENABLE
#include "protocol_esb.h"
#endif

// Current state
static struct {
    wireless_mode_t current_mode;
    wireless_mode_t target_mode;
    mode_status_t status;
    ble_slot_t ble_slot;
    bool mode_change_pending;
    uint32_t mode_change_timestamp;
    mode_change_callback_t callback;
} wm_state = {
    .current_mode = WIRELESS_MODE_USB,
    .target_mode = WIRELESS_MODE_USB,
    .status = MODE_STATUS_DISCONNECTED,
    .ble_slot = BLE_SLOT_0,
    .mode_change_pending = false,
    .mode_change_timestamp = 0,
    .callback = NULL
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

void wireless_mode_init(void) {
    // Default to USB mode
    wm_state.current_mode = WIRELESS_MODE_USB;
    wm_state.target_mode = WIRELESS_MODE_USB;
    wm_state.status = MODE_STATUS_DISCONNECTED;
    wm_state.ble_slot = BLE_SLOT_0;
    wm_state.mode_change_pending = false;
    wm_state.callback = NULL;

    DEBUG_PRINTF("[MODE] Init: %s\r\n", wireless_mode_name(wm_state.current_mode));

    // TODO: Load saved mode from EEPROM
    // wireless_mode_t saved = eeprom_read_wireless_mode();
    // if (wireless_mode_available(saved)) {
    //     wireless_mode_switch(saved);
    // }
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

    DEBUG_PRINTF("[MODE] BLE slot: %d\r\n", slot);
    wm_state.ble_slot = slot;

    // If already in BLE mode, trigger reconnection to new slot
    if (wm_state.current_mode == WIRELESS_MODE_BLE) {
#ifdef BLE_ENABLE
        ble_switch_slot(slot);
#endif
    }

    return true;
}

ble_slot_t wireless_mode_get_ble_slot(void) {
    return wm_state.ble_slot;
}

bool wireless_mode_available(wireless_mode_t mode) {
    switch (mode) {
        case WIRELESS_MODE_USB:
            // USB always available (even if not connected)
            return true;

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
            // TODO: esb_deinit();
#endif
            break;

        default:
            break;
    }
}

static void enter_mode(wireless_mode_t mode) {
    switch (mode) {
        case WIRELESS_MODE_USB:
            // USB is always initialized, just enable it
            // TODO: usb_enable();
            break;

        case WIRELESS_MODE_BLE:
#ifdef BLE_ENABLE
            // BLE is initialized at startup, just switch to the slot and advertise
            ble_switch_slot(wm_state.ble_slot);
#endif
            break;

        case WIRELESS_MODE_ESB:
#ifdef ESB_ENABLE
            // TODO: esb_init();
            // TODO: esb_start_tx();
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

    // USB mode key
    if (keycode == 0x7C00) { // USB custom keycode (to be defined properly)
        return wireless_mode_switch(WIRELESS_MODE_USB);
    }

    // 2.4G mode key
    if (keycode == 0x7C01) { // ESB custom keycode
        return wireless_mode_switch(WIRELESS_MODE_ESB);
    }

    // BLE slot keys (0x7C10 - 0x7C1F)
    if (keycode >= 0x7C10 && keycode <= 0x7C1F) {
        ble_slot_t slot = keycode - 0x7C10;
        if (slot < BLE_SLOT_MAX) {
            wireless_mode_switch_ble_slot(slot);
            return wireless_mode_switch(WIRELESS_MODE_BLE);
        }
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
