/* WCH stack storage and compatibility hooks required by the QMK adapter. */

#include "config.h"
#include "quantum.h"
#include "protocol.h"

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];
const uint8_t MacAddr[6] = {0};

/* Board-owned status_indicator.c replaces the adapter's optional indicator. */
void wireless_indicator_status_reset(void) {}
void wireless_indicator_daemon(void) {}

void wireless_rgb_indicator_task(uint8_t led_min, uint8_t led_max) {
    (void)led_min;
    (void)led_max;
}

bool wireless_pre_process_record_kb(uint16_t keycode, keyrecord_t *record) {
    (void)keycode;
    (void)record;
    return true;
}

/* Obey65 mode keys are handled by process_wireless_keycode(). */
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
