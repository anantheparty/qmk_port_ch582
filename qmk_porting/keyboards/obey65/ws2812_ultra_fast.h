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

// 超高速WS2812驱动 - 专为0.5us级别精确时序设计

// 时序宏定义 - 精确到纳秒级别
#define NOP __asm__("nop")
#define delay_nop_12()  NOP;NOP;NOP;NOP;
#define delay_nop_19()  delay_nop_12(); NOP;NOP;NOP;
#define delay_nop_38()  delay_nop_19(); delay_nop_19()
#define delay_nop_48()  delay_nop_38(); NOP;NOP;NOP;

// WS2812 时序宏 - 优化版本
#define BIT0_FAST gpio_write_pin_high(A10);\
                   gpio_write_pin_high(A11);\
                   delay_nop_12();\
                   gpio_write_pin_low(A11);\
                   gpio_write_pin_low(A10);\
                   delay_nop_48();

#define BIT1_FAST gpio_write_pin_high(A10);\
                   gpio_write_pin_high(A11);\
                   delay_nop_38();\
                   gpio_write_pin_low(A11);\
                   gpio_write_pin_low(A10);\
                   delay_nop_19();

// 函数声明
void ws2812_ultra_fast_init(void);
void ws2812_ultra_fast_send_byte(uint8_t byte);
void ws2812_ultra_fast_send_led(uint8_t r, uint8_t g, uint8_t b);
void ws2812_ultra_fast_send_4leds(uint8_t r, uint8_t g, uint8_t b);