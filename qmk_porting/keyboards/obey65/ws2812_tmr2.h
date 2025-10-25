/*
Copyright 2025 Obey65

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include <stdint.h>

// 单个LED颜色（GRB顺序的物理发送，接口使用常规RGB语义）
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} tmr2_led_t;

// 基于TMR2+DMA的WS2812（4颗）发送接口（PA11）
void tmr2_ws2812_init(void);
void tmr2_ws2812_update_4(uint8_t r, uint8_t g, uint8_t b);
void tmr2_ws2812_update_4_colors(tmr2_led_t led0, tmr2_led_t led1, tmr2_led_t led2, tmr2_led_t led3);
void tmr2_ws2812_off(void);


