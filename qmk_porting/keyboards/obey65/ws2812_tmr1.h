/*
Copyright 2025 Obey65

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include "ws2812_common.h"

// 基于TMR1+DMA的WS2812（50颗）发送接口（PA10）
void tmr1_ws2812_init(void);
void tmr1_ws2812_update_all(uint8_t r, uint8_t g, uint8_t b);
void tmr1_ws2812_update_index(uint8_t index, led_obey_t led);
void tmr1_ws2812_off(void);
