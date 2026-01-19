/*
 * Obey65 2.4G Receiver - Minimal Keymap
 *
 * The receiver has no physical keys - it receives HID reports via RF
 * and forwards them to USB. This file is required by the build system
 * but contains only an empty keymap definition.
 */

#include "quantum.h"

// Empty keymap - receiver has no matrix (MATRIX_ROWS = 0, MATRIX_COLS = 0)
const uint16_t PROGMEM keymaps[][1][1] = {
    [0] = {{ KC_NO }}
};
