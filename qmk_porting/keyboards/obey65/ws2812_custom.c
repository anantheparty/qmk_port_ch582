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

#define MAX_LIGHT 35

// 当前颜色状态 - 分开控制
ws2812_color_t current_color_4led = { 0, 0, 0 };  // 4灯带默认黑色
ws2812_color_t current_color_50led = { 0, 0, 0 }; // 50灯带默认黑色
bool ws2812_power_4led = true;                    // 4灯带电源状态
bool ws2812_power_50led = true;                   // 50灯带电源状态

// RGB整数控制状态
rgb_control_t rgb_control_4led = { 4, 4, 4, 20 };  // 4灯带RGB控制，初始值5,5,5，亮度20
rgb_control_t rgb_control_50led = { 4, 4, 4, 20 }; // 50灯带RGB控制，初始值5,5,5，亮度20

// 初始化 WS2812
void ws2812_custom_init(void)
{
    // 使用超高速驱动初始化
    ws2812_ultra_fast_init();

    ws2812_power_4led = true;
    ws2812_power_50led = true;

    // 更新LED显示
    ws2812_custom_update_led_strip(LED_STRIP_4);
    ws2812_custom_update_led_strip(LED_STRIP_50);
}

// 发送96位数据 (4个LED，每个24位) - 超高速版本
void ws2812_custom_send_96bits(void)
{
    // 兼容旧版本，同时发送两个灯带
    ws2812_custom_send_strip(LED_STRIP_4);
    ws2812_custom_send_strip(LED_STRIP_50);
}

// 发送指定灯带数据
void ws2812_custom_send_strip(led_strip_type_t strip)
{
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

// 调整指定灯带的RGB整数
void ws2812_custom_adjust_rgb_strip(uint8_t channel, int8_t delta, led_strip_type_t strip)
{
    rgb_control_t *control;

    switch (strip) {
        case LED_STRIP_4:
            control = &rgb_control_4led;
            break;
        case LED_STRIP_50:
            control = &rgb_control_50led;
            break;
        default:
            return;
    }

    switch (channel) {
        case 0: // 红色
            control->r += delta;
            if (control->r > 200)
                control->r = 0;
            if (control->r > 10)
                control->r = 10;
            break;
        case 1: // 绿色
            control->g += delta;
            if (control->g > 200)
                control->g = 0;
            if (control->g > 10)
                control->g = 10;
            break;
        case 2: // 蓝色
            control->b += delta;
            if (control->b > 200)
                control->b = 0;
            if (control->b > 10)
                control->b = 10;
            break;
    }

    // 更新LED显示
    ws2812_custom_update_led_strip(strip);
}

// 调整指定灯带的亮度
void ws2812_custom_adjust_brightness_strip(int8_t delta, led_strip_type_t strip)
{
    rgb_control_t *control;

    switch (strip) {
        case LED_STRIP_4:
            control = &rgb_control_4led;
            break;
        case LED_STRIP_50:
            control = &rgb_control_50led;
            break;
        default:
            return;
    }

    int16_t new_brightness = control->brightness + delta;
    if (new_brightness < 0)
        new_brightness = 0;
    if (new_brightness > 100)
        new_brightness = 100;

    control->brightness = (uint8_t)new_brightness;

    // 更新LED显示
    ws2812_custom_update_led_strip(strip);
}

// 获取指定灯带的RGB控制状态
rgb_control_t ws2812_custom_get_rgb_control_strip(led_strip_type_t strip)
{
    switch (strip) {
        case LED_STRIP_4:
            return rgb_control_4led;
        case LED_STRIP_50:
            return rgb_control_50led;
        default:
            return rgb_control_4led;
    }
}

// 更新指定灯带的LED显示
void ws2812_custom_update_led_strip(led_strip_type_t strip)
{
    rgb_control_t *control;
    ws2812_color_t *color;

    switch (strip) {
        case LED_STRIP_4:
            control = &rgb_control_4led;
            color = &current_color_4led;
            break;
        case LED_STRIP_50:
            control = &rgb_control_50led;
            color = &current_color_50led;
            break;
        default:
            return;
    }

    // 计算实际RGB值：RGB整数 * 亮度百分比 * MAX_LIGHT / 10
    float brightness_factor = control->brightness / 100.0f;
    color->r = (uint8_t)(control->r * brightness_factor * MAX_LIGHT / 10);
    color->g = (uint8_t)(control->g * brightness_factor * MAX_LIGHT / 10);
    color->b = (uint8_t)(control->b * brightness_factor * MAX_LIGHT / 10);

    // 发送新的颜色数据
    ws2812_custom_send_strip(strip);
}

void ws2812_custom_set_color_temp(uint8_t r, uint8_t g, uint8_t b)
{
    r %= MAX_LIGHT;
    g %= MAX_LIGHT;
    b %= MAX_LIGHT;
    // 临时设置两个灯带
    ws2812_ultra_fast_send_4leds(r, g, b);
    ws2812_ultra_fast_send_50leds(r, g, b);
}

void ws2812_toggle_power(bool power)
{
    ws2812_power_4led = power;
    ws2812_power_50led = power;
    ws2812_custom_send_strip(LED_STRIP_4);
    ws2812_custom_send_strip(LED_STRIP_50);
}

bool ws2812_custom_power_get(void)
{
    return ws2812_power_4led && ws2812_power_50led;
}

// 调整颜色通道 (兼容旧版本)
void ws2812_custom_adjust_color(uint8_t channel, int8_t delta)
{
    ws2812_custom_adjust_color_strip(channel, delta, LED_STRIP_4);
    ws2812_custom_adjust_color_strip(channel, delta, LED_STRIP_50);
}

// 调整指定灯带颜色通道
void ws2812_custom_adjust_color_strip(uint8_t channel, int8_t delta, led_strip_type_t strip)
{
    uint8_t new_value;
    ws2812_color_t *current_color;

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
        case 0: // 红色
            new_value = current_color->r + delta + MAX_LIGHT;
            new_value %= MAX_LIGHT;
            current_color->r = new_value;
            break;
        case 1: // 绿色
            new_value = current_color->g + delta + MAX_LIGHT;
            new_value %= MAX_LIGHT;
            current_color->g = new_value;
            break;
        case 2: // 蓝色
            new_value = current_color->b + delta + MAX_LIGHT;
            new_value %= MAX_LIGHT;
            current_color->b = new_value;
            break;
    }

    // 发送新的颜色数据
    ws2812_custom_send_strip(strip);
}

// 获取指定灯带当前颜色
ws2812_color_t ws2812_custom_get_current_color_strip(led_strip_type_t strip)
{
    switch (strip) {
        case LED_STRIP_4:
            return current_color_4led;
        case LED_STRIP_50:
            return current_color_50led;
        default:
            return current_color_4led;
    }
}