/**
 * Wireless stub implementations for Obey65 receiver firmware.
 *
 * Provides required symbols when ESB is enabled without BLE wireless sources.
 */

#include "config.h"
#include "quantum.h"
#include "protocol.h"

// BLE stack requires these symbols even if BLE is not actively used.
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];
const uint8_t MacAddr[6] = {0};

void wireless_indicator_status_reset(void) {
}

void wireless_indicator_daemon(void) {
}

void wireless_rgb_indicator_task(uint8_t led_min, uint8_t led_max) {
    (void)led_min;
    (void)led_max;
}

bool wireless_pre_process_record_kb(uint16_t keycode, keyrecord_t *record) {
    (void)keycode;
    (void)record;
    return true;
}

bool wireless_process_record(uint16_t keycode, keyrecord_t *record) {
    (void)keycode;
    (void)record;
    return false;
}

bool process_ble_passcode(uint16_t keycode, keyrecord_t *record) {
    (void)keycode;
    (void)record;
    return false;
}

bool process_ble_passcode_kb(uint16_t keycode, keyrecord_t *record) {
    return process_ble_passcode(keycode, record);
}
