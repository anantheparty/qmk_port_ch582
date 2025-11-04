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
} led_obey_t;
