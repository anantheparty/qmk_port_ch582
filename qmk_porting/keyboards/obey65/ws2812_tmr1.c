/*
Copyright 2025 Obey65

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
*/

#include "CH58x_common.h"
#include "pin_defs.h"
#include "ws2812_tmr1.h"

// 针对40MHz时钟调整WS2812的时序参数
// WS2812协议大致时序：（以40MHz，即25ns/周期计算）
// T1H: 0.8us ≈ 32cycles
// T0H: 0.4us ≈ 16cycles
// T_total(周期): 1.25us ≈ 50cycles

#define PERIOD_TICKS 50    // 总周期1.25us
#define T1H_TICKS    32    // 1高电平0.8us
#define T0H_TICKS    16    // 0高电平0.4us
#define RESET_L_BITS 80    // reset信号 80*1.25us=100us

#define LED_COUNT_TMR1 50
#define BITS_PER_LED 24
#define FRAME_BITS_TMR1 (LED_COUNT_TMR1 * BITS_PER_LED)
#define DMA_BUF_LEN_TMR1 (FRAME_BITS_TMR1 + RESET_L_BITS)

__attribute__ ((aligned (4))) static uint32_t TMR1_DmaBuf[DMA_BUF_LEN_TMR1];

static inline void EncodeByte(uint8_t val, uint32_t *dst) {
    for (int b = 7; b >= 0; --b) {
        *dst++ = ((val >> b) & 0x01) ? T1H_TICKS : T0H_TICKS;
    }
}

static void BuildFrameAll(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t *p = TMR1_DmaBuf;
    for (int i = 0; i < LED_COUNT_TMR1; i++) {
        EncodeByte(g, p); p += 8;
        EncodeByte(r, p); p += 8;
        EncodeByte(b, p); p += 8;
    }
    for (int i = 0; i < RESET_L_BITS; ++i) *p++ = 0;
}

static void UpdateLedAtIndex(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= LED_COUNT_TMR1) return;
    
    uint32_t *p = TMR1_DmaBuf + (index * BITS_PER_LED);
    EncodeByte(g, p); p += 8;
    EncodeByte(r, p); p += 8;
    EncodeByte(b, p);
}

static void BuildFrameCycle(void) {
    uint32_t *p = TMR1_DmaBuf;
    uint8_t colors[7][3] = {
        {7, 0, 0},   // 红色
        {0, 7, 0},   // 绿色
        {0, 0, 7},   // 蓝色
        {7, 7, 0},   // 黄色
        {7, 0, 7},   // 紫色
        {0, 7, 7},   // 青色
        {7, 7, 7}    // 白色
    };
    
    for (int i = 0; i < LED_COUNT_TMR1; i++) {
        int color_index = i % 7;
        EncodeByte(colors[color_index][1], p); p += 8; // G
        EncodeByte(colors[color_index][0], p); p += 8; // R
        EncodeByte(colors[color_index][2], p); p += 8; // B
    }
    for (int i = 0; i < RESET_L_BITS; ++i) *p++ = 0;
}

void tmr1_ws2812_init(void) {
    GPIOA_ModeCfg(GPIO_Pin_10, GPIO_ModeOut_PP_5mA);
    TMR1_PWMCycleCfg(PERIOD_TICKS);
    TMR1_DMACfg(ENABLE,(uint16_t)(uint32_t)&TMR1_DmaBuf[0],(uint16_t)(uint32_t)&TMR1_DmaBuf[DMA_BUF_LEN_TMR1],Mode_LOOP);
    TMR1_PWMInit(High_Level, PWM_Times_1);
    TMR1_PWMEnable();
    TMR1_Enable();
    BuildFrameCycle();
}

void tmr1_ws2812_update_all(uint8_t r, uint8_t g, uint8_t b) {
    BuildFrameAll(r, g, b);
}

void tmr1_ws2812_update_index(uint8_t index, led_obey_t led) {
    UpdateLedAtIndex(index, led.r, led.g, led.b);
}

void tmr1_ws2812_off(void) {
    BuildFrameAll(0, 0, 0);
}
