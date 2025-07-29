/*
Copyright 2024 Obey65

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H

// 自定义按键定义
enum custom_keycodes {
    KC_CUSTOM_0 = SAFE_RANGE,
    KC_CUSTOM_1,
    KC_CUSTOM_2,
    KC_CUSTOM_3,
    KC_FN,
    KC_BOOTLOADER_JUMP
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT_all(
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC, MO(2),
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS, KC_CUSTOM_1,
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_ENT,  KC_ENT,   KC_CUSTOM_2,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT, KC_UP,   KC_UP,   KC_CUSTOM_3,
        KC_LCTL, KC_LGUI, KC_LALT, _______,   _______,   KC_SPC,  _______,   _______,   MO(1),   KC_RCTL, _______,   _______,   KC_LEFT, KC_DOWN, KC_RGHT
    ),
    [1] = LAYOUT_all(
        KC_GRV,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_DEL,  KC_BOOTLOADER_JUMP,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,   _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,   _______,
        _______, _______, _______, _______, _______,   _______,   _______,   _______,   _______,   _______, _______, _______, _______, _______, _______
    ),
    [2] = LAYOUT_all(
        // 暂时禁用RGB控制按键，改为普通按键
        KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   _______, _______, _______, _______, _______, _______, _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______
    ),
    [3] = LAYOUT_all(
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______
    )
}; 

const uint32_t unicode_map[] = {
    [0x00] = 0x03B1, // α
    [0x01] = 0x03B2, // β
    [0x02] = 0x03B3, // γ
    [0x03] = 0x03C0, // π
    [0x04] = 0x2665, // ♥
    [0x05] = 0x2192, // →
    [0x06] = 0x00A5, // ¥
};

// 自定义按键处理函数
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_CUSTOM_0:
            if (record->event.pressed) {
                // 自定义按键0的功能
                SEND_STRING("Custom Key 0");
            }
            return false;
        case KC_CUSTOM_1:
            if (record->event.pressed) {
                // 自定义按键1的功能
                SEND_STRING("Custom Key 1");
            }
            return false;
        case KC_CUSTOM_2:
            if (record->event.pressed) {
                // 自定义按键2的功能
                SEND_STRING("Custom Key 2");
            }
            return false;
        case KC_CUSTOM_3:
            if (record->event.pressed) {
                // 自定义按键3的功能
                SEND_STRING("Custom Key 3");
            }
            return false;
        case KC_FN:
            if (record->event.pressed) {
                // Fn键功能 - 切换到层1
                layer_on(1);
            } else {
                layer_off(1);
            }
            return false;
        case KC_BOOTLOADER_JUMP:
            if (record->event.pressed) {
                // 跳转到bootloader
                bootloader_jump();
            }
            return false;
    }
    return true;
}