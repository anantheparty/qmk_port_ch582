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

#ifdef QMK_KEYBOARD_H
#include QMK_KEYBOARD_H
#else
#include "quantum.h"
#endif
#include <stdbool.h>
#include "ws2812_tmr2.h"

#ifndef RGBLED_NUM
#define RGBLED_NUM 4
#endif

#ifndef SAFE_RANGE
#define SAFE_RANGE 0x6000
#endif

#define DEBUG 1 // 调试开关：1=开启调试，0=关闭调试
#define RGB_STEP 2         // RGB调整步长
#define BRIGHTNESS_STEP 10  // 亮度调整步长

// 自定义按键定义（仅保留4灯带控制）
enum custom_keycodes {
    KC_CUSTOM_0 = SAFE_RANGE,
    KC_CUSTOM_1,
    KC_CUSTOM_2,
    KC_CUSTOM_3,
    KC_FN,
    KC_BOOTLOADER_JUMP,
    KC_RGB_DEBUG,      // 4灯调试按键
    KC_LED_INIT_LOW,   // 4灯关闭
    KC_LED_INIT_HIGH,  // 4灯开启
    KC_LED_TOGGLE,     // 4灯开关
    KC_RGB_R_MINUS,
    KC_RGB_R_PLUS,
    KC_RGB_G_MINUS,
    KC_RGB_G_PLUS,
    KC_RGB_B_MINUS,
    KC_RGB_B_PLUS,
    KC_BRIGHTNESS_MINUS,
    KC_BRIGHTNESS_PLUS,
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT_all(
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC, MO(2),
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS, _______,
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_ENT,  KC_ENT,   KC_CUSTOM_2,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT, KC_UP,   KC_UP,   KC_CUSTOM_3,
        KC_LCTL, KC_LGUI, KC_LALT, _______,   _______,   KC_SPC,  _______,   _______,   MO(1),   KC_RCTL, _______,   _______,   KC_LEFT, KC_DOWN, KC_RGHT
    ),
    [1] = LAYOUT_all(
        KC_GRV,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_DEL,  KC_BOOTLOADER_JUMP,
        _______, KC_LED_INIT_LOW, KC_LED_INIT_HIGH, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,   _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,   _______,
        _______, _______, _______, _______, _______,   _______,   _______,   _______,   _______,   _______, _______, _______, _______, _______, _______
    ),
    [2] = LAYOUT_all(
        // Layer 2: 仅4灯带RGB整数控制层
        KC_LED_TOGGLE  , KC_RGB_R_MINUS, KC_RGB_R_PLUS, KC_RGB_G_MINUS, KC_RGB_G_PLUS,  KC_RGB_B_MINUS, KC_RGB_B_PLUS, KC_BRIGHTNESS_MINUS, KC_BRIGHTNESS_PLUS,   _______,   _______,   _______,   _______,   _______,   MO(2),   
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

// 简单的 4灯 RGB 状态：整数RGB(0~10)与亮度(0~100)
static uint8_t g_r = 4, g_g = 4, g_b = 4, g_brightness = 20; // 默认值
static bool g_on = true;
static inline uint8_t scale(uint8_t v) { return (uint8_t)((v * 35 / 10) * g_brightness / 100); }
static inline uint8_t clamp_u8(int v, int minv, int maxv) { if (v < minv) return (uint8_t)minv; if (v > maxv) return (uint8_t)maxv; return (uint8_t)v; }
static void tmr2_apply(void) { if (g_on) tmr2_ws2812_update_4(scale(g_r), scale(g_g), scale(g_b)); else tmr2_ws2812_off(); }

// 调整并输出调试信息
static void adjust_rgb_and_debug(uint8_t channel, int8_t adjustment) {
    switch (channel) {
        case 0: g_r = clamp_u8((int)g_r + adjustment, 0, 10); break;
        case 1: g_g = clamp_u8((int)g_g + adjustment, 0, 10); break;
        case 2: g_b = clamp_u8((int)g_b + adjustment, 0, 10); break;
    }
    tmr2_apply();
#if DEBUG
    char buf[96];
    snprintf(buf, sizeof(buf), "4LED RGB(%u,%u,%u) Bright:%u -> Actual(%u,%u,%u)\r\n", g_r, g_g, g_b, g_brightness, scale(g_r), scale(g_g), scale(g_b));
    SEND_STRING(buf);
#endif
}

static void adjust_brightness_and_debug(int8_t adjustment) {
    int v = (int)g_brightness + adjustment;
    if (v < 0) {
        v = 0;
    }
    if (v > 100) {
        v = 100;
    }
    g_brightness = (uint8_t)v;
    tmr2_apply();
#if DEBUG
    char buf[64];
    snprintf(buf, sizeof(buf), "4LED Brightness:%u\r\n", g_brightness);
    SEND_STRING(buf);
#endif
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;
    switch (keycode) {
        case KC_FN: layer_on(1); return false;
        case KC_BOOTLOADER_JUMP: bootloader_jump(); return false;
        case KC_RGB_DEBUG:
#if DEBUG
            {
                char buf[96];
                snprintf(buf, sizeof(buf), "DBG 4LED RGB(%u,%u,%u) Bright:%u -> Actual(%u,%u,%u)\r\n", g_r, g_g, g_b, g_brightness, scale(g_r), scale(g_g), scale(g_b));
                SEND_STRING(buf);
            }
#endif
            return false;
        case KC_LED_TOGGLE:
            g_on = !g_on; tmr2_apply(); return false;
        case KC_LED_INIT_LOW:  g_on = false; tmr2_apply(); return false;
        case KC_LED_INIT_HIGH: g_on = true;  tmr2_apply(); return false;
        case KC_RGB_R_MINUS:       adjust_rgb_and_debug(0, -RGB_STEP); return false;
        case KC_RGB_R_PLUS:        adjust_rgb_and_debug(0,  RGB_STEP); return false;
        case KC_RGB_G_MINUS:       adjust_rgb_and_debug(1, -RGB_STEP); return false;
        case KC_RGB_G_PLUS:        adjust_rgb_and_debug(1,  RGB_STEP); return false;
        case KC_RGB_B_MINUS:       adjust_rgb_and_debug(2, -RGB_STEP); return false;
        case KC_RGB_B_PLUS:        adjust_rgb_and_debug(2,  RGB_STEP); return false;
        case KC_BRIGHTNESS_MINUS:  adjust_brightness_and_debug(-BRIGHTNESS_STEP); return false;
        case KC_BRIGHTNESS_PLUS:   adjust_brightness_and_debug( BRIGHTNESS_STEP); return false;
    }
    return true;
}