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

#include QMK_KEYBOARD_H
#include "led.h"
#include "ws2812_custom.h"

#ifdef RGB_MATRIX_ENABLE
// RGB Matrix LED 位置配置 - 四颗 LED 竖着排列在右 CTRL 右侧
led_config_t g_led_config = { { // Key Matrix to LED Index
                                { NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED },
                                { NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED },
                                { NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED },
                                { NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED },
                                { NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED } },
                              { // LED Index to Physical Position - 四颗 LED 竖着排列在键盘右侧
                                // LED 0: 最上面
                                { 220, 10 },
                                // LED 1: 第二个
                                { 220, 25 },
                                // LED 2: 第三个
                                { 220, 40 },
                                // LED 3: 最下面
                                { 220, 55 } },
                              { // LED Index to Flag - 所有 LED 都是装饰灯
                                LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW } };
#endif

// 大写锁定LED初始化
void keyboard_pre_init_kb(void)
{
    // 设置大写锁定LED引脚为输出
    gpio_set_pin_output(LED_CAPS_LOCK_PIN);
    gpio_write_pin_low(LED_CAPS_LOCK_PIN);
    gpio_set_pin_output(A11);
    gpio_write_pin_high(A11);
    keyboard_pre_init_user();
}

// 大写锁定LED更新函数
bool led_update_kb(led_t led_state)
{
    bool res = led_update_user(led_state);
    if (res) {
        // 根据大写锁定状态控制LED
        if (led_state.caps_lock) {
            gpio_write_pin_high(LED_CAPS_LOCK_PIN);
        } else {
            gpio_write_pin_low(LED_CAPS_LOCK_PIN);
        }
    }
    return res;
}

void keyboard_post_init_kb(void)
{
    // 初始化自定义WS2812控制
    ws2812_custom_init();
    keyboard_post_init_user();
    SEND_STRING("Custom WS2812 INIT - 4LED & 50LED Separated\r\n");
}

int main()
{
    extern void protocol_setup();
    extern void protocol_pre_init();
    extern void protocol_post_init();
    extern void platform_run();

    platform_setup();

    protocol_setup();
#if !defined ESB_ENABLE || ESB_ENABLE != 2
    keyboard_setup();
#endif
    gpio_set_pin_output(A11);
    gpio_set_pin_output(A10);
    gpio_write_pin_low(A11);
    gpio_write_pin_low(A10);
    protocol_pre_init();
    keyboard_init();
    protocol_post_init();

    // // 初始化两个灯带
    // DelayUs(150);
    
    /* Main loop */
    for (;;) {
        platform_run();
        //! housekeeping_task() is handled by platform
    }
}