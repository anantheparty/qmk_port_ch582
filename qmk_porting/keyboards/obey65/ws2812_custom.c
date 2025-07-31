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
#include "eeprom.h"

// 当前颜色状态
ws2812_color_t current_color = {128, 128, 128};  // 默认白色
bool ws2812_power = true;  // 电源状态

// EEPROM 地址定义
#define EEPROM_COLOR_ADDR 0x1000

// 时序宏定义
#define NOP __asm__("nop")
#define delay_nop_12()  NOP;NOP;NOP;NOP;
#define delay_nop_19()  delay_nop_12(); NOP;NOP;NOP;
#define delay_nop_38()  delay_nop_19(); delay_nop_19()
#define delay_nop_48()  delay_nop_38(); NOP;NOP;NOP;

// WS2812 时序宏
#define BIT0 gpio_write_pin_high(A10);\
              gpio_write_pin_high(A11);\
              delay_nop_12();\
              gpio_write_pin_low(A11);\
              gpio_write_pin_low(A10);\
              delay_nop_48();

#define BIT1 gpio_write_pin_high(A10);\
              gpio_write_pin_high(A11);\
              delay_nop_38();\
              gpio_write_pin_low(A11);\
              gpio_write_pin_low(A10);\
              delay_nop_19();

// 超高速字节发送函数 - 完全展开，无循环，无查表
static inline void ws2812_send_byte_ultra_fast(uint8_t byte) {
    // 使用位操作直接展开，避免任何循环和条件判断
    // 每个位独立处理，编译器会优化为最快的代码
    
    // 位7 (MSB)
    if (byte & 0x80) {
        BIT1;
    } else {
        BIT0;
    }
    
    // 位6
    if (byte & 0x40) {
        BIT1;
    } else {
        BIT0;
    }
    
    // 位5
    if (byte & 0x20) {
        BIT1;
    } else {
        BIT0;
    }
    
    // 位4
    if (byte & 0x10) {
        BIT1;
    } else {
        BIT0;
    }
    
    // 位3
    if (byte & 0x08) {
        BIT1;
    } else {
        BIT0;
    }
    
    // 位2
    if (byte & 0x04) {
        BIT1;
    } else {
        BIT0;
    }
    
    // 位1
    if (byte & 0x02) {
        BIT1;
    } else {
        BIT0;
    }
    
    // 位0 (LSB)
    if (byte & 0x01) {
        BIT1;
    } else {
        BIT0;
    }
}

// 发送单个LED的24位颜色数据 (GRB顺序) - 超高速版本
static inline void send_led_color_ultra_fast(ws2812_color_t color) {
    // 绿色字节 (8位)
    ws2812_send_byte_ultra_fast(color.g);
    // 红色字节 (8位)
    ws2812_send_byte_ultra_fast(color.r);
    // 蓝色字节 (8位)
    ws2812_send_byte_ultra_fast(color.b);
}

// 初始化 WS2812
void ws2812_custom_init(void) {
    // 使用超高速驱动初始化
    ws2812_ultra_fast_init();
    
    ws2812_power = true;

    // 从EEPROM加载保存的颜色
    ws2812_custom_load_from_eeprom();
    
    // 发送初始颜色
    ws2812_custom_send_96bits();
}

// 发送96位数据 (4个LED，每个24位) - 超高速版本
void ws2812_custom_send_96bits(void) {
    // 使用超高速驱动发送4个LED
    ws2812_ultra_fast_send_4leds(current_color.r, current_color.g, current_color.b);
}

// 设置颜色
void ws2812_custom_set_color(uint8_t r, uint8_t g, uint8_t b) {
    current_color.r = r;
    current_color.g = g;
    current_color.b = b;
    
    // 发送新的颜色数据
    ws2812_custom_send_96bits();
    
    // 保存到EEPROM
    ws2812_custom_save_to_eeprom();
}

void ws2812_custom_set_color_temp(uint8_t r, uint8_t g, uint8_t b) {
    // 使用超高速驱动发送4个LED
    ws2812_ultra_fast_send_4leds(r, g, b);
}

void ws2812_toggle_power(bool power) {
    if (power) {
        ws2812_custom_set_color_temp(0, 0, 0);
    } else {
        ws2812_custom_send_96bits();
    }
}

bool ws2812_custom_power_get(void) {
    return ws2812_power;
}

// 调整颜色通道
void ws2812_custom_adjust_color(uint8_t channel, int8_t delta) {
    uint8_t new_value;
    
    switch (channel) {
        case 0:  // 红色
            new_value = current_color.r + delta;
            if (new_value <= 255) {
                current_color.r = new_value;
            }
            break;
        case 1:  // 绿色
            new_value = current_color.g + delta;
            if (new_value <= 255) {
                current_color.g = new_value;
            }
            break;
        case 2:  // 蓝色
            new_value = current_color.b + delta;
            if (new_value <= 255) {
                current_color.b = new_value;
            }
            break;
    }
    
    // 发送新的颜色数据
    ws2812_custom_send_96bits();
    
    // 保存到EEPROM
    ws2812_custom_save_to_eeprom();
}

// 保存颜色到EEPROM
void ws2812_custom_save_to_eeprom(void) {
    eeprom_update_byte((uint8_t*)EEPROM_COLOR_ADDR, current_color.r);
    eeprom_update_byte((uint8_t*)EEPROM_COLOR_ADDR + 1, current_color.g);
    eeprom_update_byte((uint8_t*)EEPROM_COLOR_ADDR + 2, current_color.b);
}

// 从EEPROM加载颜色
void ws2812_custom_load_from_eeprom(void) {
    uint8_t r = eeprom_read_byte((uint8_t*)EEPROM_COLOR_ADDR);
    uint8_t g = eeprom_read_byte((uint8_t*)EEPROM_COLOR_ADDR + 1);
    uint8_t b = eeprom_read_byte((uint8_t*)EEPROM_COLOR_ADDR + 2);
    
    // 检查EEPROM是否为空 (0xFF表示未初始化)
    if (r == 0xFF && g == 0xFF && b == 0xFF) {
        // 使用默认颜色
        current_color.r = 128;
        current_color.g = 128;
        current_color.b = 128;
    } else {
        current_color.r = r;
        current_color.g = g;
        current_color.b = b;
    }
}

// 获取当前颜色
ws2812_color_t ws2812_custom_get_current_color(void) {
    return current_color;
} 