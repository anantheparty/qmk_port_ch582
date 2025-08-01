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

#include "ws2812_ultra_fast.h"
#include "quantum.h"

// 初始化超高速WS2812驱动
void ws2812_ultra_fast_init(void) {
    // 设置引脚为输出
    gpio_set_pin_output(A11);
    gpio_set_pin_output(A10);
    
    // 初始状态设为低电平
    gpio_write_pin_low(A11);
    gpio_write_pin_low(A10);
}

// 超高速字节发送 - 完全展开，无循环，无条件判断 (A11引脚)
inline void ws2812_ultra_fast_send_byte(uint8_t byte) {
    // 使用位操作直接展开，编译器会优化为最快的代码
    // 每个位独立处理，避免任何循环和复杂的条件判断
    
    // 位7 (MSB) - 绿色最高位
    if (byte & 0x80) {
        BIT1_FAST_LAST_A11();
    } else {
        BIT0_FAST_LAST_A11();
    }
    
    // 位6
    if (byte & 0x40) {
        BIT1_FAST_LAST_A11();
    } else {
        BIT0_FAST_LAST_A11();
    }
    
    // 位5
    if (byte & 0x20) {
        BIT1_FAST_LAST_A11();
    } else {
        BIT0_FAST_LAST_A11();
    }
    
    // 位4
    if (byte & 0x10) {
        BIT1_FAST_LAST_A11();
    } else {
        BIT0_FAST_LAST_A11();
    }
    
    // 位3
    if (byte & 0x08) {
        BIT1_FAST_LAST_A11();
    } else {
        BIT0_FAST_LAST_A11();
    }
    
    // 位2
    if (byte & 0x04) {
        BIT1_FAST_LAST_A11();
    } else {
        BIT0_FAST_LAST_A11();
    }
    
    // 位1
    if (byte & 0x02) {
        BIT1_FAST_LAST_A11();
    } else {
        BIT0_FAST_LAST_A11();
    }
    
    // 位0 (LSB) - 绿色最低位
    if (byte & 0x01) {
        BIT1_FAST_LAST_A11();
    } else {
        BIT0_FAST_LAST_A11();
    }
}

// 超高速字节发送 - 完全展开，无循环，无条件判断 (A10引脚)
inline void ws2812_ultra_fast_send_byte10(uint8_t byte) {
    // 使用位操作直接展开，编译器会优化为最快的代码
    // 每个位独立处理，避免任何循环和复杂的条件判断
    
    // 位7 (MSB) - 绿色最高位
    if (byte & 0x80) {
        BIT1_FAST_LAST_A10();
    } else {
        BIT0_FAST_LAST_A10();
    }
    
    // 位6
    if (byte & 0x40) {
        BIT1_FAST_LAST_A10();
    } else {
        BIT0_FAST_LAST_A10();
    }
    
    // 位5
    if (byte & 0x20) {
        BIT1_FAST_LAST_A10();
    } else {
        BIT0_FAST_LAST_A10();
    }
    
    // 位4
    if (byte & 0x10) {
        BIT1_FAST_LAST_A10();
    } else {
        BIT0_FAST_LAST_A10();
    }
    
    // 位3
    if (byte & 0x08) {
        BIT1_FAST_LAST_A10();
    } else {
        BIT0_FAST_LAST_A10();
    }
    
    // 位2
    if (byte & 0x04) {
        BIT1_FAST_LAST_A10();
    } else {
        BIT0_FAST_LAST_A10();
    }
    
    // 位1
    if (byte & 0x02) {
        BIT1_FAST_LAST_A10();
    } else {
        BIT0_FAST_LAST_A10();
    }
    
    // 位0 (LSB) - 绿色最低位
    if (byte & 0x01) {
        BIT1_FAST_LAST_A10();
    } else {
        BIT0_FAST_LAST_A10();
    }
}

// 发送单个LED的24位颜色数据 (GRB顺序) - A11引脚
inline void ws2812_ultra_fast_send_led(uint8_t r, uint8_t g, uint8_t b) {
    // 绿色字节 (8位) - 第一个发送
    ws2812_ultra_fast_send_byte(g);
    // 红色字节 (8位) - 第二个发送
    ws2812_ultra_fast_send_byte(r);
    // 蓝色字节 (8位) - 第三个发送
    ws2812_ultra_fast_send_byte(b);
}

// 发送单个LED的24位颜色数据 (GRB顺序) - A10引脚
inline void ws2812_ultra_fast_send_led10(uint8_t r, uint8_t g, uint8_t b) {
    // 绿色字节 (8位) - 第一个发送
    ws2812_ultra_fast_send_byte10(g);
    // 红色字节 (8位) - 第二个发送
    ws2812_ultra_fast_send_byte10(r);
    // 蓝色字节 (8位) - 第三个发送
    ws2812_ultra_fast_send_byte10(b);
}


// 发送4个LED的颜色数据 - 完全展开，无循环 (A11引脚)
inline void ws2812_ultra_fast_send_4leds(uint8_t r, uint8_t g, uint8_t b) {
    // LED 0
    ws2812_ultra_fast_send_led(r, g, b);
    // LED 1
    ws2812_ultra_fast_send_led(r, g, b);
    // LED 2
    ws2812_ultra_fast_send_led(r, g, b);
    // LED 3
    ws2812_ultra_fast_send_led(r, g, b);
    
    // 发送复位信号 (至少50us低电平)
    gpio_write_pin_low(A11);
    
    // 延时约50us (在48MHz下约2400个nop)
    DelayUs(150);
}

// 发送50个LED的颜色数据 - 完全展开，无循环 (A10引脚)
void ws2812_ultra_fast_send_50leds(uint8_t r, uint8_t g, uint8_t b) {
    
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);

    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);

    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);

    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);

    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);

    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);

    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);

    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);

    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);

    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    ws2812_ultra_fast_send_led10(r, g, b);
    
    // 发送复位信号 (至少50us低电平)
    gpio_write_pin_low(A10);
    
    // 延时约50us (在48MHz下约2400个nop)
    DelayUs(150);
}