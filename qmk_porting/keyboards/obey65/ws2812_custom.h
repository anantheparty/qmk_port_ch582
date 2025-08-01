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

#pragma once

#include <stdint.h>
#include <stdbool.h>

// WS2812 颜色结构体
typedef struct {
    uint8_t g;  // 绿色 (GRB顺序)
    uint8_t r;  // 红色
    uint8_t b;  // 蓝色
} ws2812_color_t;

// 颜色调整步长
#define COLOR_STEP_SMALL  1
#define COLOR_STEP_LARGE  5

// 灯带类型定义
typedef enum {
    LED_STRIP_4 = 0,   // 4灯带
    LED_STRIP_50 = 1   // 50灯带
} led_strip_type_t;

// 函数声明
void ws2812_custom_init(void);
void ws2812_custom_send_96bits(void);
void ws2812_custom_set_color(uint8_t r, uint8_t g, uint8_t b);
void ws2812_custom_set_color_temp(uint8_t r, uint8_t g, uint8_t b);
void ws2812_toggle_power(bool power);
void ws2812_custom_adjust_color(uint8_t channel, int8_t delta);
void ws2812_custom_save_to_eeprom(void);
void ws2812_custom_load_from_eeprom(void);
ws2812_color_t ws2812_custom_get_current_color(void);
bool ws2812_custom_power_get(void);

// 新增：分开控制函数
void ws2812_custom_set_color_strip(uint8_t r, uint8_t g, uint8_t b, led_strip_type_t strip);
void ws2812_custom_adjust_color_strip(uint8_t channel, int8_t delta, led_strip_type_t strip);
ws2812_color_t ws2812_custom_get_current_color_strip(led_strip_type_t strip);
void ws2812_custom_save_to_eeprom_strip(led_strip_type_t strip);
void ws2812_custom_load_from_eeprom_strip(led_strip_type_t strip);
void ws2812_custom_send_strip(led_strip_type_t strip);

// 外部变量声明
extern ws2812_color_t current_color_4led; 
extern ws2812_color_t current_color_50led;
extern bool ws2812_power_4led;
extern bool ws2812_power_50led;