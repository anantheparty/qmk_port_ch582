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
#define delay_nop_4()  NOP;NOP;NOP;NOP;
#define delay_nop_6()  delay_nop_4(); NOP;NOP;
#define delay_nop_7()  delay_nop_4(); NOP;NOP;NOP;
#define delay_nop_13()  delay_nop_7(); delay_nop_4(); NOP;NOP; 
#define delay_nop_15()  delay_nop_7(); delay_nop_4(); NOP;NOP; NOP;NOP;
#define delay_nop_16()  delay_nop_13(); NOP;NOP;

// WS2812 时序宏 - 优化版本（A10/A11通用）
#define BIT0_FAST(pin) gpio_write_pin_high(pin); \
                       delay_nop_4(); \
                       gpio_write_pin_low(pin); \
                       delay_nop_16();

#define BIT0_FAST_LAST(pin) gpio_write_pin_high(pin); \
                       delay_nop_4(); \
                       gpio_write_pin_low(pin); \
                       delay_nop_13();
                    

#define BIT1_FAST(pin) gpio_write_pin_high(pin); \
                       delay_nop_13(); \
                       gpio_write_pin_low(pin); \
                       delay_nop_7();

#define BIT1_FAST_LAST(pin) gpio_write_pin_high(pin); \
                       delay_nop_13(); \
                       gpio_write_pin_low(pin); \
                       delay_nop_4();

// 兼容旧代码的宏定义
#define BIT0_FAST_A11 BIT0_FAST(A11)
#define BIT1_FAST_A11 BIT1_FAST(A11)
#define BIT0_FAST_A10 BIT0_FAST(A10)
#define BIT1_FAST_A10 BIT1_FAST(A10)

// 函数声明
void ws2812_ultra_fast_init(void);
void ws2812_ultra_fast_send_byte(uint8_t byte);
void ws2812_ultra_fast_send_led(uint8_t r, uint8_t g, uint8_t b);
void ws2812_ultra_fast_send_4leds(uint8_t r, uint8_t g, uint8_t b);
void ws2812_ultra_fast_send_byte10(uint8_t byte);
void ws2812_ultra_fast_send_led10(uint8_t r, uint8_t g, uint8_t b);
void ws2812_ultra_fast_send_50leds(uint8_t r, uint8_t g, uint8_t b);