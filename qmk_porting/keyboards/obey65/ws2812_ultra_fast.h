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
#define delay_nop_3()  NOP;NOP;NOP;
#define delay_nop_4()  NOP;NOP;NOP;NOP;
#define delay_nop_6()  delay_nop_4(); NOP;NOP;
#define delay_nop_7()  delay_nop_4(); NOP;NOP;NOP;
#define delay_nop_12() delay_nop_7(); delay_nop_4(); NOP;
#define delay_nop_13() delay_nop_7(); delay_nop_4(); NOP;NOP; 
#define delay_nop_15() delay_nop_7(); delay_nop_4(); NOP;NOP; NOP;NOP;
#define delay_nop_16() delay_nop_13(); NOP;NOP;NOP;

// A11引脚时序宏 - 1.25us周期，1是0.85+0.4，0是0.25+1
#define BIT0_FAST_A11() gpio_write_pin_high(A11); \
                       delay_nop_4(); \
                       gpio_write_pin_low(A11); \
                       delay_nop_16();

#define BIT0_FAST_LAST_A11() gpio_write_pin_high(A11); \
                       delay_nop_4(); \
                       gpio_write_pin_low(A11); \
                       delay_nop_13();
                    

#define BIT1_FAST_A11() gpio_write_pin_high(A11); \
                       delay_nop_13(); \
                       gpio_write_pin_low(A11); \
                       delay_nop_7();

#define BIT1_FAST_LAST_A11() gpio_write_pin_high(A11); \
                       delay_nop_13(); \
                       gpio_write_pin_low(A11); \
                       delay_nop_4();

// A10引脚时序宏 - 1.05us周期，1是0.75+0.3，0是0.3+0.75
#define BIT0_FAST_A10() gpio_write_pin_high(A10); \
                       delay_nop_6(); \
                       gpio_write_pin_low(A10); \
                       delay_nop_15();

#define BIT0_FAST_LAST_A10() gpio_write_pin_high(A10); \
                       delay_nop_6(); \
                       gpio_write_pin_low(A10); \
                       delay_nop_12();
                    

#define BIT1_FAST_A10() gpio_write_pin_high(A10); \
                       delay_nop_15(); \
                       gpio_write_pin_low(A10); \
                       delay_nop_6();

#define BIT1_FAST_LAST_A10() gpio_write_pin_high(A10); \
                       delay_nop_15(); \
                       gpio_write_pin_low(A10); \
                       delay_nop_3();

// 函数声明
void ws2812_ultra_fast_init(void);
void ws2812_ultra_fast_send_byte(uint8_t byte);
void ws2812_ultra_fast_send_led(uint8_t r, uint8_t g, uint8_t b);
void ws2812_ultra_fast_send_4leds(uint8_t r, uint8_t g, uint8_t b);
void ws2812_ultra_fast_send_byte10(uint8_t byte);
void ws2812_ultra_fast_send_led10(uint8_t r, uint8_t g, uint8_t b);
void ws2812_ultra_fast_send_50leds(uint8_t r, uint8_t g, uint8_t b);
