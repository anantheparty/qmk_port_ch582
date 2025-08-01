/*
Copyright 2025 Obey65

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

#include "ws2812_custom.h"
#include "ws2812_ultra_fast.h"
#include "quantum.h"
#include "eeprom.h"

#define MAX_LIGHT 35
#define INIT_LIGHT 18

// 当前颜色状态 - 分开控制
ws2812_color_t current_color_4led = {INIT_LIGHT, INIT_LIGHT, INIT_LIGHT};   // 4灯带默认白色
ws2812_color_t current_color_50led = {INIT_LIGHT, INIT_LIGHT, INIT_LIGHT};  // 50灯带默认白色
bool ws2812_power_4led = true;   // 4灯带电源状态
bool ws2812_power_50led = true;  // 50灯带电源状态

// EEPROM 地址定义
#define EEPROM_COLOR_4LED_ADDR  0x1000
#define EEPROM_COLOR_50LED_ADDR 0x1010

// 初始化 WS2812
void ws2812_custom_init(void) {
    // 使用超高速驱动初始化
    ws2812_ultra_fast_init();
    
    ws2812_power_4led = true;
    ws2812_power_50led = true;

    // 从EEPROM加载保存的颜色
    ws2812_custom_load_from_eeprom_strip(LED_STRIP_4);
    ws2812_custom_load_from_eeprom_strip(LED_STRIP_50);
    
    // 发送初始颜色
    ws2812_custom_send_strip(LED_STRIP_4);
    ws2812_custom_send_strip(LED_STRIP_50);
}

// 发送96位数据 (4个LED，每个24位) - 超高速版本
void ws2812_custom_send_96bits(void) {
    // 兼容旧版本，同时发送两个灯带
    ws2812_custom_send_strip(LED_STRIP_4);
    ws2812_custom_send_strip(LED_STRIP_50);
}

// 发送指定灯带数据
void ws2812_custom_send_strip(led_strip_type_t strip) {
    switch (strip) {
        case LED_STRIP_4:
            if (ws2812_power_4led) {
                ws2812_ultra_fast_send_4leds(current_color_4led.r, current_color_4led.g, current_color_4led.b);
            } else {
                ws2812_ultra_fast_send_4leds(0, 0, 0);
            }
            break;
        case LED_STRIP_50:
            if (ws2812_power_50led) {
                ws2812_ultra_fast_send_50leds(current_color_50led.r, current_color_50led.g, current_color_50led.b);
            } else {
                ws2812_ultra_fast_send_50leds(0, 0, 0);
            }
            break;
    }
}

// 设置颜色 (兼容旧版本)
void ws2812_custom_set_color(uint8_t r, uint8_t g, uint8_t b) {
    ws2812_custom_set_color_strip(r, g, b, LED_STRIP_4);
    ws2812_custom_set_color_strip(r, g, b, LED_STRIP_50);
}

// 设置指定灯带颜色
void ws2812_custom_set_color_strip(uint8_t r, uint8_t g, uint8_t b, led_strip_type_t strip) {
    r %= MAX_LIGHT;
    g %= MAX_LIGHT;
    b %= MAX_LIGHT;
    
    switch (strip) {
        case LED_STRIP_4:
            current_color_4led.r = r;
            current_color_4led.g = g;
            current_color_4led.b = b;
            break;
        case LED_STRIP_50:
            current_color_50led.r = r;
            current_color_50led.g = g;
            current_color_50led.b = b;
            break;
    }
    
    // 发送新的颜色数据
    ws2812_custom_send_strip(strip);
    
    // 保存到EEPROM
    ws2812_custom_save_to_eeprom_strip(strip);
}

void ws2812_custom_set_color_temp(uint8_t r, uint8_t g, uint8_t b) {
    r %= MAX_LIGHT;
    g %= MAX_LIGHT;
    b %= MAX_LIGHT;
    // 临时设置两个灯带
    ws2812_ultra_fast_send_4leds(r, g, b);
    ws2812_ultra_fast_send_50leds(r, g, b);
}

void ws2812_toggle_power(bool power) {
    ws2812_power_4led = power;
    ws2812_power_50led = power;
    ws2812_custom_send_strip(LED_STRIP_4);
    ws2812_custom_send_strip(LED_STRIP_50);
}

bool ws2812_custom_power_get(void) {
    return ws2812_power_4led && ws2812_power_50led;
}

// 调整颜色通道 (兼容旧版本)
void ws2812_custom_adjust_color(uint8_t channel, int8_t delta) {
    ws2812_custom_adjust_color_strip(channel, delta, LED_STRIP_4);
    ws2812_custom_adjust_color_strip(channel, delta, LED_STRIP_50);
}

// 调整指定灯带颜色通道
void ws2812_custom_adjust_color_strip(uint8_t channel, int8_t delta, led_strip_type_t strip) {
    uint8_t new_value;
    ws2812_color_t* current_color;
    
    switch (strip) {
        case LED_STRIP_4:
            current_color = &current_color_4led;
            break;
        case LED_STRIP_50:
            current_color = &current_color_50led;
            break;
        default:
            return;
    }
    
    switch (channel) {
        case 0:  // 红色
            new_value = current_color->r + delta + MAX_LIGHT;
            new_value %= MAX_LIGHT;
            current_color->r = new_value;
            break;
        case 1:  // 绿色
            new_value = current_color->g + delta + MAX_LIGHT;
            new_value %= MAX_LIGHT;
            current_color->g = new_value;
            break;
        case 2:  // 蓝色
            new_value = current_color->b + delta + MAX_LIGHT;
            new_value %= MAX_LIGHT;
            current_color->b = new_value;
            break;
    }
    
    // 发送新的颜色数据
    ws2812_custom_send_strip(strip);
    
    // 保存到EEPROM
    ws2812_custom_save_to_eeprom_strip(strip);
}

// 保存颜色到EEPROM (兼容旧版本)
void ws2812_custom_save_to_eeprom(void) {
    ws2812_custom_save_to_eeprom_strip(LED_STRIP_4);
    ws2812_custom_save_to_eeprom_strip(LED_STRIP_50);
}

// 保存指定灯带颜色到EEPROM
void ws2812_custom_save_to_eeprom_strip(led_strip_type_t strip) {
    uint16_t addr;
    ws2812_color_t* color;
    
    switch (strip) {
        case LED_STRIP_4:
            addr = EEPROM_COLOR_4LED_ADDR;
            color = &current_color_4led;
            break;
        case LED_STRIP_50:
            addr = EEPROM_COLOR_50LED_ADDR;
            color = &current_color_50led;
            break;
        default:
            return;
    }
    
    eeprom_update_byte((uint8_t*)(uintptr_t)addr, color->r);
    eeprom_update_byte((uint8_t*)(uintptr_t)(addr + 1), color->g);
    eeprom_update_byte((uint8_t*)(uintptr_t)(addr + 2), color->b);
}

// 从EEPROM加载颜色 (兼容旧版本)
void ws2812_custom_load_from_eeprom(void) {
    ws2812_custom_load_from_eeprom_strip(LED_STRIP_4);
    ws2812_custom_load_from_eeprom_strip(LED_STRIP_50);
}

// 从EEPROM加载指定灯带颜色
void ws2812_custom_load_from_eeprom_strip(led_strip_type_t strip) {
    uint16_t addr;
    ws2812_color_t* color;
    
    switch (strip) {
        case LED_STRIP_4:
            addr = EEPROM_COLOR_4LED_ADDR;
            color = &current_color_4led;
            break;
        case LED_STRIP_50:
            addr = EEPROM_COLOR_50LED_ADDR;
            color = &current_color_50led;
            break;
        default:
            return;
    }
    
    uint8_t r = eeprom_read_byte((uint8_t*)(uintptr_t)addr);
    uint8_t g = eeprom_read_byte((uint8_t*)(uintptr_t)(addr + 1));
    uint8_t b = eeprom_read_byte((uint8_t*)(uintptr_t)(addr + 2));
    
    // 检查EEPROM是否为空 (0xFF表示未初始化)
    if (r == 0xFF && g == 0xFF && b == 0xFF) {
        // 使用默认颜色
        color->r = INIT_LIGHT;
        color->g = INIT_LIGHT;
        color->b = INIT_LIGHT;
    } else {
        color->r = r % MAX_LIGHT;
        color->g = g % MAX_LIGHT;
        color->b = b % MAX_LIGHT;
    }
}

// 获取当前颜色 (兼容旧版本)
ws2812_color_t ws2812_custom_get_current_color(void) {
    return current_color_4led; // 返回4灯带颜色作为默认
}

// 获取指定灯带当前颜色
ws2812_color_t ws2812_custom_get_current_color_strip(led_strip_type_t strip) {
    switch (strip) {
        case LED_STRIP_4:
            return current_color_4led;
        case LED_STRIP_50:
            return current_color_50led;
        default:
            return current_color_4led;
    }
} 