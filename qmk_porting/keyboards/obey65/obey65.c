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

#include "quantum.h"
#include "led.h"
#include "ws2812_tmr2.h"
#include "qmk_config.h"
#include "pin_defs.h"

#ifndef LED_CAPS_LOCK_PIN
#define LED_CAPS_LOCK_PIN (0x80000000 | GPIO_Pin_17)
#endif
#ifndef A10
#define A10 (GPIO_Pin_10)
#endif
#ifndef A11
#define A11 (GPIO_Pin_11)
#endif

#ifdef RGB_MATRIX_ENABLE
// RGB Matrix LED 位置配置 - 50颗 LED 沿一条直线排列（从左到右）
led_config_t g_led_config = { { // Key Matrix to LED Index
                               { NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED },
                               { NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED },
                               { NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED },
                               { NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED },
                               { NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED, NO_LED } },
                             { // LED Index to Physical Position - 线性分布在 (x:0..224, y:8)
                               {   0,  8 }, {   5,  8 }, {  10,  8 }, {  15,  8 }, {  20,  8 },
                               {  25,  8 }, {  30,  8 }, {  35,  8 }, {  40,  8 }, {  45,  8 },
                               {  50,  8 }, {  55,  8 }, {  60,  8 }, {  65,  8 }, {  70,  8 },
                               {  75,  8 }, {  80,  8 }, {  85,  8 }, {  90,  8 }, {  95,  8 },
                               { 100,  8 }, { 105,  8 }, { 110,  8 }, { 115,  8 }, { 120,  8 },
                               { 125,  8 }, { 130,  8 }, { 135,  8 }, { 140,  8 }, { 145,  8 },
                               { 150,  8 }, { 155,  8 }, { 160,  8 }, { 165,  8 }, { 170,  8 },
                               { 175,  8 }, { 180,  8 }, { 185,  8 }, { 190,  8 }, { 195,  8 },
                               { 200,  8 }, { 205,  8 }, { 210,  8 }, { 215,  8 }, { 220,  8 },
                               { 225,  8 }, { 230,  8 }, { 235,  8 }, { 240,  8 }, { 245,  8 } },
                             { // LED Index to Flag - 全部为装饰灯
                               LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW,
                               LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW,
                               LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW,
                               LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW,
                               LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW,
                               LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW,
                               LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW,
                               LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW,
                               LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW,
                               LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW, LED_FLAG_UNDERGLOW } };
#endif

// 大写锁定LED初始化
void keyboard_pre_init_kb(void)
{
    // 设置大写锁定LED引脚为输出
#ifdef LED_CAPS_LOCK_PIN
    gpio_set_pin_output(LED_CAPS_LOCK_PIN);
    gpio_write_pin_low(LED_CAPS_LOCK_PIN);
#endif
    // 自定义TMR2由对应模块初始化
    keyboard_pre_init_user();
}

// 大写锁定LED更新函数
bool led_update_kb(led_t led_state)
{
    bool res = led_update_user(led_state);
    if (res) {
        // 根据大写锁定状态控制LED
#ifdef LED_CAPS_LOCK_PIN
        if (led_state.caps_lock) {
            gpio_write_pin_high(LED_CAPS_LOCK_PIN);
        } else {
            gpio_write_pin_low(LED_CAPS_LOCK_PIN);
        }
#endif
    }
    return res;
}

void keyboard_post_init_kb(void)
{
    // 初始化4LED的TMR2控制
    tmr2_ws2812_init();
    keyboard_post_init_user();
    SEND_STRING("TMR2 WS2812 (4LED) INIT\r\n");
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
    protocol_pre_init();
    keyboard_init();
    protocol_post_init();

    // // 初始化两个灯带
    // DelayUs(150);
    
    // // 初始化4灯带
    // ws2812_ultra_fast_send_4leds(7, 7, 7);
    
    // // 初始化50灯带
    // gpio_write_pin_high(A10);
    // DelayUs(150);
    // gpio_write_pin_low(A10);
    // DelayUs(150);
    
    // // 发送50灯带数据
    // for (int i = 0; i < 10; i++) {
    //     ws2812_ultra_fast_send_50leds(8, 8, 8);
    //     gpio_write_pin_low(A10);
    //     DelayUs(150);
    // }

    /* Main loop */
    for (;;) {
        platform_run();
        //! housekeeping_task() is handled by platform
    }
}