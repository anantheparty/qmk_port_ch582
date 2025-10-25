/*
Copyright 2025 Obey65

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
*/

#include "CH58x_common.h"
#include "pin_defs.h"
#include "ws2812_tmr2.h"

#define PERIOD_TICKS 75
#define T1H_TICKS 40
#define T0H_TICKS 20
#define RESET_L_BITS 80

#define LED_COUNT_TMR2 4
#define BITS_PER_LED 24
#define FRAME_BITS_TMR2 (LED_COUNT_TMR2 * BITS_PER_LED)
#define DMA_BUF_LEN_TMR2 (FRAME_BITS_TMR2 + RESET_L_BITS)

__attribute__ ((aligned (4))) static uint32_t TMR2_DmaBuf[DMA_BUF_LEN_TMR2];

static inline void EncodeByte(uint8_t val, uint32_t *dst) {
    for (int b = 7; b >= 0; --b) {
        *dst++ = ((val >> b) & 0x01) ? T1H_TICKS : T0H_TICKS;
    }
}

static void BuildFrame4(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t *p = TMR2_DmaBuf;
    for (int i = 0; i < LED_COUNT_TMR2; i++) {
        EncodeByte(g, p); p += 8;
        EncodeByte(r, p); p += 8;
        EncodeByte(b, p); p += 8;
    }
    for (int i = 0; i < RESET_L_BITS; ++i) *p++ = 0;
}

static void BuildFrame4Colors(uint8_t r0, uint8_t g0, uint8_t b0,
                              uint8_t r1, uint8_t g1, uint8_t b1,
                              uint8_t r2, uint8_t g2, uint8_t b2,
                              uint8_t r3, uint8_t g3, uint8_t b3) {
    uint32_t *p = TMR2_DmaBuf;
    // led0
    EncodeByte(g0, p); p += 8; EncodeByte(r0, p); p += 8; EncodeByte(b0, p); p += 8;
    // led1
    EncodeByte(g1, p); p += 8; EncodeByte(r1, p); p += 8; EncodeByte(b1, p); p += 8;
    // led2
    EncodeByte(g2, p); p += 8; EncodeByte(r2, p); p += 8; EncodeByte(b2, p); p += 8;
    // led3
    EncodeByte(g3, p); p += 8; EncodeByte(r3, p); p += 8; EncodeByte(b3, p); p += 8;
    for (int i = 0; i < RESET_L_BITS; ++i) *p++ = 0;
}

void tmr2_ws2812_init(void) {
    GPIOA_ModeCfg(GPIO_Pin_11, GPIO_ModeOut_PP_5mA);
    TMR2_PWMCycleCfg(PERIOD_TICKS);
    TMR2_DMACfg(ENABLE,(uint16_t)(uint32_t)&TMR2_DmaBuf[0],(uint16_t)(uint32_t)&TMR2_DmaBuf[DMA_BUF_LEN_TMR2],Mode_LOOP);
    TMR2_PWMInit(High_Level, PWM_Times_1);
    TMR2_PWMEnable();
    TMR2_Enable();
}

void tmr2_ws2812_update_4(uint8_t r, uint8_t g, uint8_t b) {
    BuildFrame4(r, g, b);
}

void tmr2_ws2812_update_4_colors(tmr2_led_t led0, tmr2_led_t led1, tmr2_led_t led2, tmr2_led_t led3) {
    BuildFrame4Colors(led0.r, led0.g, led0.b,
                      led1.r, led1.g, led1.b,
                      led2.r, led2.g, led2.b,
                      led3.r, led3.g, led3.b);
}

void tmr2_ws2812_off(void) {
    BuildFrame4(0, 0, 0);
}


