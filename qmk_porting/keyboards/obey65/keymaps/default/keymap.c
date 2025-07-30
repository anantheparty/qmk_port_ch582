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
#include "ws2812.h"

#ifndef RGBLED_NUM
#define RGBLED_NUM 4
#endif
// 自定义按键定义
enum custom_keycodes {
    KC_CUSTOM_0 = SAFE_RANGE,
    KC_CUSTOM_1,
    KC_CUSTOM_2,
    KC_CUSTOM_3,
    KC_FN,
    KC_BOOTLOADER_JUMP,
    KC_RGB_DEBUG,  // RGB调试按键
    KC_LED_INIT_LOW,  // LED状态显示按键
    KC_LED_INIT_HIGH,    // LED初始化按键
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
        // Layer 2: RGB 控制层
        RGB_TOG, RGB_MOD, RGB_RMOD, RGB_HUI, RGB_HUD, RGB_SAI, RGB_SAD, RGB_VAI, RGB_VAD, RGB_SPI, RGB_SPD, KC_RGB_DEBUG, KC_LED_INIT_LOW, KC_LED_INIT_HIGH, _______,
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



// RGB 按键调试处理函数
bool process_record_user_rgb(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        switch (keycode) {
            case RGB_TOG:
                SEND_STRING("RGB_TOG: Toggle RGB\r\n");
                if (!ws2812_power_get()) {
                    SEND_STRING("WS2812 Power Off\r\n");
                } else {
                    SEND_STRING("WS2812 Power On\r\n");
                }
                // ws2812_power_toggle(!ws2812_power_get());
                break;
            case RGB_MOD:
                SEND_STRING("RGB_MOD: Next Mode\r\n");
                break;
            case RGB_RMOD:
                SEND_STRING("RGB_RMOD: Previous Mode\r\n");
                break;
            case RGB_HUI:
                SEND_STRING("RGB_HUI: Hue +10\r\n");
                break;
            case RGB_HUD:
                SEND_STRING("RGB_HUD: Hue -10\r\n");
                break;
            case RGB_SAI:
                SEND_STRING("RGB_SAI: Saturation +8\r\n");
                break;
            case RGB_SAD:
                SEND_STRING("RGB_SAD: Saturation -8\r\n");
                break;
            case RGB_VAI:
                SEND_STRING("RGB_VAI: Value +4\r\n");
                break;
            case RGB_VAD:
                SEND_STRING("RGB_VAD: Value -4\r\n");
                break;
            case RGB_SPI:
                SEND_STRING("RGB_SPI: Speed +10\r\n");
                break;
            case RGB_SPD:
                SEND_STRING("RGB_SPD: Speed -10\r\n");
                break;
        }
    }
    return true;
}

// 重写 process_record_user 函数来包含 RGB 调试
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // 处理自定义按键
    switch (keycode) {
        case KC_CUSTOM_0:
            if (record->event.pressed) {
                SEND_STRING("Custom Key 0\r\n");
            }
            return false;
        case KC_CUSTOM_1:
            if (record->event.pressed) {
                SEND_STRING("Custom Key 1\r\n");
            }
            return false;
        case KC_CUSTOM_2:
            if (record->event.pressed) {
                SEND_STRING("Custom Key 2\r\n");
            }
            return false;
        case KC_CUSTOM_3:
            if (record->event.pressed) {
                SEND_STRING("Custom Key 3\r\n");
            }
            return false;
        case KC_FN:
            if (record->event.pressed) {
                layer_on(1);
            } else {
                layer_off(1);
            }
            return false;
        case KC_BOOTLOADER_JUMP:
            if (record->event.pressed) {
                bootloader_jump();
            }
            return false;
        case KC_RGB_DEBUG:
            if (record->event.pressed) {
                SEND_STRING("RGB Debug: ");
                #ifdef RGB_MATRIX_ENABLE
                SEND_STRING("RGB Matrix Enabled, ");
                #else
                SEND_STRING("RGB Matrix Disabled, ");
                #endif
                #ifdef WS2812_DRIVER_PWM
                SEND_STRING("PWM Driver, ");
                #endif
                #ifdef WS2812_DRIVER_SPI
                SEND_STRING("SPI Driver, ");
                #endif
                #if WS2812_DI_PIN == PA10
                    SEND_STRING("PA10 Pin\r\n");
                #else
                    SEND_STRING("PA11 Pin\r\n");
                #endif
            }
            return false;
        // case KC_LED_STATUS:
        //     if (record->event.pressed) {
        //         SEND_STRING("LED Status: ");
        //         #ifdef RGB_MATRIX_ENABLE
        //         SEND_STRING("RGB Matrix Active, ");
        //         #ifdef RGB_MATRIX_MAXIMUM_BRIGHTNESS
        //         char brightness_str[25];
        //         snprintf(brightness_str, sizeof(brightness_str), "Max Brightness: %d, ", RGB_MATRIX_MAXIMUM_BRIGHTNESS);
        //         SEND_STRING(brightness_str);
        //         #endif
        //         #ifdef RGBLED_NUM
        //         char led_num_str[20];
        //         snprintf(led_num_str, sizeof(led_num_str), "LED Count: %d, ", RGBLED_NUM);
        //         SEND_STRING(led_num_str);
        //         #endif
        //         #else
        //         SEND_STRING("RGB Matrix Inactive, ");
        //         #endif
        //         SEND_STRING("WS2812 PWM TMR2\r\n");
        //         if (!ws2812_power_get()) {
        //             SEND_STRING("WS2812 Power Off\r\n");
        //         } else {
        //             SEND_STRING("WS2812 Power On\r\n");
        //         }
        //     }
        //     return false;
        case KC_LED_INIT_LOW:
            if (record->event.pressed) {
                ws2812_power_toggle(false);
            }
            return false;
        case KC_LED_INIT_HIGH:
            if (record->event.pressed) {
                ws2812_power_toggle(true);
            }
            return false;
    }
    
    // 处理 RGB 按键调试
    return process_record_user_rgb(keycode, record);
}