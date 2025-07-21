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

// 大写锁定LED初始化
void keyboard_pre_init_kb(void) {
    // 设置大写锁定LED引脚为输出
    gpio_set_pin_output(LED_CAPS_LOCK_PIN);
    gpio_write_pin_low(LED_CAPS_LOCK_PIN);
    
    keyboard_pre_init_user();
}

// 大写锁定LED更新函数
bool led_update_kb(led_t led_state) {
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

    /* Main loop */
    for (;;) {
        platform_run();
        //! housekeeping_task() is handled by platform
    }
} 