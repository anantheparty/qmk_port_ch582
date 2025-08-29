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
#include "ws2812_custom.h"

#ifndef RGBLED_NUM
#define RGBLED_NUM 4
#endif

#define DEBUG 1 // 调试开关：1=开启调试，0=关闭调试
#define RGB_STEP 2         // RGB调整步长
#define BRIGHTNESS_STEP 10  // 亮度调整步长

// 自定义按键定义
enum custom_keycodes {
    KC_CUSTOM_0 = SAFE_RANGE,
    KC_CUSTOM_1,
    KC_CUSTOM_2,
    KC_CUSTOM_3,
    KC_FN,
    KC_BOOTLOADER_JUMP,
    KC_RGB_DEBUG,  // RGB调试按键
    KC_LED_INIT_LOW,  // LED关闭
    KC_LED_INIT_HIGH,  // LED开启
    KC_LED_TOGGLE,     // LED开关
    // 50灯带控制按键
    KC_50_LED_TOGGLE,     // 50灯带开关
    KC_50_LED_INIT_LOW,   // 50灯带关闭
    KC_50_LED_INIT_HIGH,  // 50灯带开启
    // 新的RGB整数和亮度控制按键
    KC_RGB_R_MINUS,       // RGB红色 -1
    KC_RGB_R_PLUS,        // RGB红色 +1
    KC_RGB_G_MINUS,       // RGB绿色 -1
    KC_RGB_G_PLUS,        // RGB绿色 +1
    KC_RGB_B_MINUS,       // RGB蓝色 -1
    KC_RGB_B_PLUS,        // RGB蓝色 +1
    KC_BRIGHTNESS_MINUS,  // 亮度 -5
    KC_BRIGHTNESS_PLUS,   // 亮度 +5
    // 50灯带RGB整数和亮度控制按键
    KC_50_RGB_R_MINUS,       // 50灯带RGB红色 -1
    KC_50_RGB_R_PLUS,        // 50灯带RGB红色 +1
    KC_50_RGB_G_MINUS,       // 50灯带RGB绿色 -1
    KC_50_RGB_G_PLUS,        // 50灯带RGB绿色 +1
    KC_50_RGB_B_MINUS,       // 50灯带RGB蓝色 -1
    KC_50_RGB_B_PLUS,        // 50灯带RGB蓝色 +1
    KC_50_BRIGHTNESS_MINUS,  // 50灯带亮度 -5
    KC_50_BRIGHTNESS_PLUS,   // 50灯带亮度 +5
    // Off/On逻辑测试按键
    KC_4LED_OFF_SIGNAL,      // 4灯带Off信号
    KC_4LED_ON_SIGNAL,       // 4灯带On信号
    KC_50LED_OFF_SIGNAL,     // 50灯带Off信号
    KC_50LED_ON_SIGNAL,      // 50灯带On信号
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT_all(
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC, MO(2),
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS, MO(3),
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_ENT,  KC_ENT,   KC_CUSTOM_2,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT, KC_UP,   KC_UP,   KC_CUSTOM_3,
        KC_LCTL, KC_LGUI, KC_LALT, _______,   _______,   KC_SPC,  _______,   _______,   MO(1),   KC_RCTL, _______,   _______,   KC_LEFT, KC_DOWN, KC_RGHT
    ),
    [1] = LAYOUT_all(
        KC_GRV,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_DEL,  KC_BOOTLOADER_JUMP,
        _______, KC_4LED_OFF_SIGNAL, KC_4LED_ON_SIGNAL, KC_50LED_OFF_SIGNAL, KC_50LED_ON_SIGNAL, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,   _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,   _______,
        _______, _______, _______, _______, _______,   _______,   _______,   _______,   _______,   _______, _______, _______, _______, _______, _______
    ),
    [2] = LAYOUT_all(
        // Layer 2: 4灯带RGB整数控制层
        RGB_TOG, RGB_RMOD, RGB_MOD, RGB_HUD, RGB_HUI, RGB_SAD, RGB_SAI, RGB_VAD, RGB_VAI, _______, _______, _______, _______, _______, MO(2),
        KC_LED_INIT_LOW,   KC_LED_INIT_HIGH,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______
    ),
    [3] = LAYOUT_all(
        // Layer 3: 50灯带RGB整数控制层
        KC_50_LED_TOGGLE  , KC_50_RGB_R_MINUS, KC_50_RGB_R_PLUS, KC_50_RGB_G_MINUS, KC_50_RGB_G_PLUS,  KC_50_RGB_B_MINUS, KC_50_RGB_B_PLUS, KC_50_BRIGHTNESS_MINUS, KC_50_BRIGHTNESS_PLUS, _______,  _______, _______,   _______,   _______,   MO(3),   
        KC_50_LED_INIT_LOW,   KC_50_LED_INIT_HIGH,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
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

// 辅助函数：调整RGB整数并输出调试信息
static void adjust_rgb_and_debug(uint8_t channel, int8_t adjustment, const char* color_name, led_strip_type_t strip) {
    ws2812_custom_adjust_rgb_strip(channel, adjustment, strip);
#if DEBUG
    rgb_control_t current_rgb = ws2812_custom_get_rgb_control_strip(strip);
    ws2812_color_t actual_color = ws2812_custom_get_current_color_strip(strip);
    char debug_str[120];
    const char* strip_name = (strip == LED_STRIP_4) ? "4LED" : "50LED";
    snprintf(debug_str, sizeof(debug_str), "%s %s %+d -> RGB(%d,%d,%d) Bright:%d -> Actual(%d,%d,%d)\r\n", 
             strip_name, color_name, adjustment, current_rgb.r, current_rgb.g, current_rgb.b, current_rgb.brightness,
             actual_color.r, actual_color.g, actual_color.b);
    SEND_STRING(debug_str);
#endif
}

// 辅助函数：调整亮度并输出调试信息
static void adjust_brightness_and_debug(int8_t adjustment, led_strip_type_t strip) {
    ws2812_custom_adjust_brightness_strip(adjustment, strip);
#if DEBUG
    rgb_control_t current_rgb = ws2812_custom_get_rgb_control_strip(strip);
    ws2812_color_t actual_color = ws2812_custom_get_current_color_strip(strip);
    char debug_str[120];
    const char* strip_name = (strip == LED_STRIP_4) ? "4LED" : "50LED";
    snprintf(debug_str, sizeof(debug_str), "%s Brightness %+d -> RGB(%d,%d,%d) Bright:%d -> Actual(%d,%d,%d)\r\n", 
             strip_name, adjustment, current_rgb.r, current_rgb.g, current_rgb.b, current_rgb.brightness,
             actual_color.r, actual_color.g, actual_color.b);
    SEND_STRING(debug_str);
#endif
}

// 重写 process_record_user 函数来包含 RGB 调试
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return true;
    }

    // 处理自定义按键
    switch (keycode) {
        case KC_CUSTOM_0:
#if DEBUG
            SEND_STRING("Custom Key 0\r\n");
#endif
            return false;
        case KC_CUSTOM_1:
#if DEBUG
            SEND_STRING("Custom Key 1\r\n");
#endif
            return false;
        case KC_CUSTOM_2:
#if DEBUG
            SEND_STRING("Custom Key 2\r\n");
#endif
            return false;
        case KC_CUSTOM_3:
#if DEBUG
            SEND_STRING("Custom Key 3\r\n");
#endif
            return false;
        case KC_FN:
            layer_on(1);
            return false;
        case KC_BOOTLOADER_JUMP:
            bootloader_jump();
            return false;
        case KC_RGB_DEBUG:
#if DEBUG
            {
                rgb_control_t rgb_4led = ws2812_custom_get_rgb_control_strip(LED_STRIP_4);
                rgb_control_t rgb_50led = ws2812_custom_get_rgb_control_strip(LED_STRIP_50);
                ws2812_color_t actual_4led = ws2812_custom_get_current_color_strip(LED_STRIP_4);
                ws2812_color_t actual_50led = ws2812_custom_get_current_color_strip(LED_STRIP_50);
                char debug_str[200];
                snprintf(debug_str, sizeof(debug_str), "RGB Debug: 4LED RGB(%d,%d,%d) Bright:%d -> Actual(%d,%d,%d), 50LED RGB(%d,%d,%d) Bright:%d -> Actual(%d,%d,%d)\r\n", 
                         rgb_4led.r, rgb_4led.g, rgb_4led.b, rgb_4led.brightness, actual_4led.r, actual_4led.g, actual_4led.b,
                         rgb_50led.r, rgb_50led.g, rgb_50led.b, rgb_50led.brightness, actual_50led.r, actual_50led.g, actual_50led.b);
                SEND_STRING(debug_str);
            }
#endif
            return false;
        // 4灯带控制
        case KC_LED_TOGGLE:
            {
                extern bool ws2812_power_4led;
                ws2812_power_4led = !ws2812_power_4led;
                if (ws2812_power_4led) {
#if DEBUG
                    SEND_STRING("4LED Toggle: ON\r\n");
#endif
                    ws2812_custom_send_on_signal(LED_STRIP_4);
                } else {
#if DEBUG
                    SEND_STRING("4LED Toggle: OFF\r\n");
#endif
                    ws2812_custom_send_off_signal(LED_STRIP_4);
                }
            }
            return false;
        case KC_LED_INIT_LOW:
#if DEBUG
            SEND_STRING("4LED OFF\r\n");
#endif
            ws2812_custom_send_off_signal(LED_STRIP_4);
            return false;
        case KC_LED_INIT_HIGH:
#if DEBUG
            SEND_STRING("4LED ON\r\n");
#endif
            ws2812_custom_send_on_signal(LED_STRIP_4);
            return false;
        // 4灯带RGB整数控制
        case KC_RGB_R_MINUS:
            adjust_rgb_and_debug(0, -RGB_STEP, "Red", LED_STRIP_4);
            return false;
        case KC_RGB_R_PLUS:
            adjust_rgb_and_debug(0, RGB_STEP, "Red", LED_STRIP_4);
            return false;
        case KC_RGB_G_MINUS:
            adjust_rgb_and_debug(1, -RGB_STEP, "Green", LED_STRIP_4);
            return false;
        case KC_RGB_G_PLUS:
            adjust_rgb_and_debug(1, RGB_STEP, "Green", LED_STRIP_4);
            return false;
        case KC_RGB_B_MINUS:
            adjust_rgb_and_debug(2, -RGB_STEP, "Blue", LED_STRIP_4);
            return false;
        case KC_RGB_B_PLUS:
            adjust_rgb_and_debug(2, RGB_STEP, "Blue", LED_STRIP_4);
            return false;
        case KC_BRIGHTNESS_MINUS:
            adjust_brightness_and_debug(-BRIGHTNESS_STEP, LED_STRIP_4);
            return false;
        case KC_BRIGHTNESS_PLUS:
            adjust_brightness_and_debug(BRIGHTNESS_STEP, LED_STRIP_4);
            return false;
        // 50灯带控制
        case KC_50_LED_TOGGLE:
            {
                extern bool ws2812_power_50led;
                ws2812_power_50led = !ws2812_power_50led;
                if (ws2812_power_50led) {
#if DEBUG
                    SEND_STRING("50LED Toggle: ON\r\n");
#endif
                    ws2812_custom_send_on_signal(LED_STRIP_50);
                } else {
#if DEBUG
                    SEND_STRING("50LED Toggle: OFF\r\n");
#endif
                    ws2812_custom_send_off_signal(LED_STRIP_50);
                }
            }
            return false;
        case KC_50_LED_INIT_LOW:
#if DEBUG
            SEND_STRING("50LED OFF\r\n");
#endif
            ws2812_custom_send_off_signal(LED_STRIP_50);
            return false;
        case KC_50_LED_INIT_HIGH:
#if DEBUG
            SEND_STRING("50LED ON\r\n");
#endif
            ws2812_custom_send_on_signal(LED_STRIP_50);
            return false;
        // 50灯带RGB整数控制
        case KC_50_RGB_R_MINUS:
            adjust_rgb_and_debug(0, -RGB_STEP, "Red", LED_STRIP_50);
            return false;
        case KC_50_RGB_R_PLUS:
            adjust_rgb_and_debug(0, RGB_STEP, "Red", LED_STRIP_50);
            return false;
        case KC_50_RGB_G_MINUS:
            adjust_rgb_and_debug(1, -RGB_STEP, "Green", LED_STRIP_50);
            return false;
        case KC_50_RGB_G_PLUS:
            adjust_rgb_and_debug(1, RGB_STEP, "Green", LED_STRIP_50);
            return false;
        case KC_50_RGB_B_MINUS:
            adjust_rgb_and_debug(2, -RGB_STEP, "Blue", LED_STRIP_50);
            return false;
        case KC_50_RGB_B_PLUS:
            adjust_rgb_and_debug(2, RGB_STEP, "Blue", LED_STRIP_50);
            return false;
        case KC_50_BRIGHTNESS_MINUS:
            adjust_brightness_and_debug(-BRIGHTNESS_STEP, LED_STRIP_50);
            return false;
        case KC_50_BRIGHTNESS_PLUS:
            adjust_brightness_and_debug(BRIGHTNESS_STEP, LED_STRIP_50);
            return false;
        // Off/On逻辑测试按键
        case KC_4LED_OFF_SIGNAL:
#if DEBUG
            SEND_STRING("4LED Off Signal\r\n");
#endif
            ws2812_custom_send_off_signal(LED_STRIP_4);
            return false;
        case KC_4LED_ON_SIGNAL:
#if DEBUG
            SEND_STRING("4LED On Signal\r\n");
#endif
            ws2812_custom_send_on_signal(LED_STRIP_4);
            return false;
        case KC_50LED_OFF_SIGNAL:
#if DEBUG
            SEND_STRING("50LED Off Signal\r\n");
#endif
            ws2812_custom_send_off_signal(LED_STRIP_50);
            return false;
        case KC_50LED_ON_SIGNAL:
#if DEBUG
            SEND_STRING("50LED On Signal\r\n");
#endif
            ws2812_custom_send_on_signal(LED_STRIP_50);
            return false;
    }
    return true;
}