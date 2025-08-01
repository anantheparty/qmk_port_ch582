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
    KC_RED_MINUS_5,    // 红色 -5
    KC_RED_MINUS_1,    // 红色 -1
    KC_RED_PLUS_1,     // 红色 +1
    KC_RED_PLUS_5,     // 红色 +5
    KC_GREEN_MINUS_5,  // 绿色 -5
    KC_GREEN_MINUS_1,  // 绿色 -1
    KC_GREEN_PLUS_1,   // 绿色 +1
    KC_GREEN_PLUS_5,   // 绿色 +5
    KC_BLUE_MINUS_5,   // 蓝色 -5
    KC_BLUE_MINUS_1,   // 蓝色 -1
    KC_BLUE_PLUS_1,    // 蓝色 +1
    KC_BLUE_PLUS_5,    // 蓝色 +5
    // 50灯带控制按键
    KC_50_RED_MINUS_5,    // 50灯带红色 -5
    KC_50_RED_MINUS_1,    // 50灯带红色 -1
    KC_50_RED_PLUS_1,     // 50灯带红色 +1
    KC_50_RED_PLUS_5,     // 50灯带红色 +5
    KC_50_GREEN_MINUS_5,  // 50灯带绿色 -5
    KC_50_GREEN_MINUS_1,  // 50灯带绿色 -1
    KC_50_GREEN_PLUS_1,   // 50灯带绿色 +1
    KC_50_GREEN_PLUS_5,   // 50灯带绿色 +5
    KC_50_BLUE_MINUS_5,   // 50灯带蓝色 -5
    KC_50_BLUE_MINUS_1,   // 50灯带蓝色 -1
    KC_50_BLUE_PLUS_1,    // 50灯带蓝色 +1
    KC_50_BLUE_PLUS_5,    // 50灯带蓝色 +5
    KC_50_LED_TOGGLE,     // 50灯带开关
    KC_50_LED_INIT_LOW,   // 50灯带关闭
    KC_50_LED_INIT_HIGH,  // 50灯带开启
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
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,   _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,   _______,
        _______, _______, _______, _______, _______,   _______,   _______,   _______,   _______,   _______, _______, _______, _______, _______, _______
    ),
    [2] = LAYOUT_all(
        // Layer 2: 4灯带RGB控制层
        KC_LED_TOGGLE  , KC_RED_MINUS_5, KC_RED_MINUS_1, KC_RED_PLUS_1, KC_RED_PLUS_5,  KC_GREEN_MINUS_5, KC_GREEN_MINUS_1, KC_GREEN_PLUS_1, KC_GREEN_PLUS_5,   KC_BLUE_MINUS_5, KC_BLUE_MINUS_1, KC_BLUE_PLUS_1, KC_BLUE_PLUS_5,   _______,   MO(2),   

        _______,   _______,   _______,   _______,   _______,  
        _______,   _______,   _______,   _______,   KC_BLUE_MINUS_5, KC_BLUE_MINUS_1, KC_BLUE_PLUS_1, KC_BLUE_PLUS_5,   _______,   _______,
        
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______, _______,   _______,   _______,   _______
    ),
    [3] = LAYOUT_all(
        // Layer 3: 50灯带RGB控制层
        KC_50_LED_TOGGLE  , KC_50_RED_MINUS_5, KC_50_RED_MINUS_1, KC_50_RED_PLUS_1, KC_50_RED_PLUS_5,  KC_50_GREEN_MINUS_5, KC_50_GREEN_MINUS_1, KC_50_GREEN_PLUS_1, KC_50_GREEN_PLUS_5, _______,  _______,
        KC_50_LED_INIT_LOW,   KC_50_LED_INIT_HIGH,   _______,   _______,   
        
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,        KC_50_BLUE_MINUS_5, KC_50_BLUE_MINUS_1, KC_50_BLUE_PLUS_1, KC_50_BLUE_PLUS_5,  _______, _______,  
        
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,
        _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______,   _______ ,_______,   _______,   _______,   _______
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

// 辅助函数：调整颜色并输出调试信息
static void adjust_color_and_debug(uint8_t channel, int8_t adjustment, const char* color_name, led_strip_type_t strip) {
    ws2812_custom_adjust_color_strip(channel, adjustment, strip);
    ws2812_color_t current_color = ws2812_custom_get_current_color_strip(strip);
    char debug_str[80];
    const char* strip_name = (strip == LED_STRIP_4) ? "4LED" : "50LED";
    snprintf(debug_str, sizeof(debug_str), "%s %s %+d -> R:%d G:%d B:%d\r\n", 
             strip_name, color_name, adjustment, current_color.r, current_color.g, current_color.b);
    SEND_STRING(debug_str);
}

// 辅助函数：处理LED电源控制
static void handle_led_power(bool power_state, const char* action, led_strip_type_t strip) {
    if (strip == LED_STRIP_4) {
        // 4灯带电源控制
        extern bool ws2812_power_4led;
        ws2812_power_4led = power_state;
        ws2812_custom_send_strip(LED_STRIP_4);
    } else {
        // 50灯带电源控制
        extern bool ws2812_power_50led;
        ws2812_power_50led = power_state;
        ws2812_custom_send_strip(LED_STRIP_50);
    }
    
    const char* strip_name = (strip == LED_STRIP_4) ? "4LED" : "50LED";
    SEND_STRING(strip_name);
    SEND_STRING(" Power: ");
    SEND_STRING(power_state ? "ON" : "OFF");
    SEND_STRING("\r\n");
}

// 重写 process_record_user 函数来包含 RGB 调试
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return true;
    }

    // 处理自定义按键
    switch (keycode) {
        case KC_CUSTOM_0:
            SEND_STRING("Custom Key 0\r\n");
            return false;
        case KC_CUSTOM_1:
            SEND_STRING("Custom Key 1\r\n");
            return false;
        case KC_CUSTOM_2:
            SEND_STRING("Custom Key 2\r\n");
            return false;
        case KC_CUSTOM_3:
            SEND_STRING("Custom Key 3\r\n");
            return false;
        case KC_FN:
            layer_on(1);
            return false;
        case KC_BOOTLOADER_JUMP:
            bootloader_jump();
            return false;
        case KC_RGB_DEBUG:
            {
                ws2812_color_t color_4led = ws2812_custom_get_current_color_strip(LED_STRIP_4);
                ws2812_color_t color_50led = ws2812_custom_get_current_color_strip(LED_STRIP_50);
                char debug_str[100];
                snprintf(debug_str, sizeof(debug_str), "RGB Debug: 4LED R:%d G:%d B:%d, 50LED R:%d G:%d B:%d\r\n", 
                         color_4led.r, color_4led.g, color_4led.b, color_50led.r, color_50led.g, color_50led.b);
                SEND_STRING(debug_str);
            }
            return false;
        // 4灯带控制
        case KC_LED_TOGGLE:
            {
                extern bool ws2812_power_4led;
                handle_led_power(!ws2812_power_4led, "Toggle", LED_STRIP_4);
            }
            return false;
        case KC_LED_INIT_LOW:
            handle_led_power(false, "OFF", LED_STRIP_4);
            return false;
        case KC_LED_INIT_HIGH:
            handle_led_power(true, "ON", LED_STRIP_4);
            return false;
        // 4灯带红色通道控制
        case KC_RED_MINUS_5:
            adjust_color_and_debug(0, -5, "Red", LED_STRIP_4);
            return false;
        case KC_RED_MINUS_1:
            adjust_color_and_debug(0, -1, "Red", LED_STRIP_4);
            return false;
        case KC_RED_PLUS_1:
            adjust_color_and_debug(0, 1, "Red", LED_STRIP_4);
            return false;
        case KC_RED_PLUS_5:
            adjust_color_and_debug(0, 5, "Red", LED_STRIP_4);
            return false;
        // 4灯带绿色通道控制
        case KC_GREEN_MINUS_5:
            adjust_color_and_debug(1, -5, "Green", LED_STRIP_4);
            return false;
        case KC_GREEN_MINUS_1:
            adjust_color_and_debug(1, -1, "Green", LED_STRIP_4);
            return false;
        case KC_GREEN_PLUS_1:
            adjust_color_and_debug(1, 1, "Green", LED_STRIP_4);
            return false;
        case KC_GREEN_PLUS_5:
            adjust_color_and_debug(1, 5, "Green", LED_STRIP_4);
            return false;
        // 4灯带蓝色通道控制
        case KC_BLUE_MINUS_5:
            adjust_color_and_debug(2, -5, "Blue", LED_STRIP_4);
            return false;
        case KC_BLUE_MINUS_1:
            adjust_color_and_debug(2, -1, "Blue", LED_STRIP_4);
            return false;
        case KC_BLUE_PLUS_1:
            adjust_color_and_debug(2, 1, "Blue", LED_STRIP_4);
            return false;
        case KC_BLUE_PLUS_5:
            adjust_color_and_debug(2, 5, "Blue", LED_STRIP_4);
            return false;
        // 50灯带控制
        case KC_50_LED_TOGGLE:
            {
                extern bool ws2812_power_50led;
                handle_led_power(!ws2812_power_50led, "Toggle", LED_STRIP_50);
            }
            return false;
        case KC_50_LED_INIT_LOW:
            handle_led_power(false, "OFF", LED_STRIP_50);
            return false;
        case KC_50_LED_INIT_HIGH:
            handle_led_power(true, "ON", LED_STRIP_50);
            return false;
        // 50灯带红色通道控制
        case KC_50_RED_MINUS_5:
            adjust_color_and_debug(0, -5, "Red", LED_STRIP_50);
            return false;
        case KC_50_RED_MINUS_1:
            adjust_color_and_debug(0, -1, "Red", LED_STRIP_50);
            return false;
        case KC_50_RED_PLUS_1:
            adjust_color_and_debug(0, 1, "Red", LED_STRIP_50);
            return false;
        case KC_50_RED_PLUS_5:
            adjust_color_and_debug(0, 5, "Red", LED_STRIP_50);
            return false;
        // 50灯带绿色通道控制
        case KC_50_GREEN_MINUS_5:
            adjust_color_and_debug(1, -5, "Green", LED_STRIP_50);
            return false;
        case KC_50_GREEN_MINUS_1:
            adjust_color_and_debug(1, -1, "Green", LED_STRIP_50);
            return false;
        case KC_50_GREEN_PLUS_1:
            adjust_color_and_debug(1, 1, "Green", LED_STRIP_50);
            return false;
        case KC_50_GREEN_PLUS_5:
            adjust_color_and_debug(1, 5, "Green", LED_STRIP_50);
            return false;
        // 50灯带蓝色通道控制
        case KC_50_BLUE_MINUS_5:
            adjust_color_and_debug(2, -5, "Blue", LED_STRIP_50);
            return false;
        case KC_50_BLUE_MINUS_1:
            adjust_color_and_debug(2, -1, "Blue", LED_STRIP_50);
            return false;
        case KC_50_BLUE_PLUS_1:
            adjust_color_and_debug(2, 1, "Blue", LED_STRIP_50);
            return false;
        case KC_50_BLUE_PLUS_5:
            adjust_color_and_debug(2, 5, "Blue", LED_STRIP_50);
            return false;
    }
    return true;
}