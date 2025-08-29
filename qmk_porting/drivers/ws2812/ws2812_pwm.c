#include "quantum.h"
#include "ws2812.h"
#include "ws2812_supplement.h"
#include "print.h"

/*
 * Dual PWM/DMA WS2812 driver for CH582M.
 * Supports two independent LED strips:
 *  - Strip 1: PA10 / TMR1
 *  - Strip 2: PA11 / TMR2
 */

#ifdef WS2812_RGBW
#define WS2812_CHANNELS 4
#else
#define WS2812_CHANNELS 3
#endif

#ifndef WS2812_PWM_DRIVER_1
#define WS2812_PWM_DRIVER_1 1
#define WS2812_DI_PIN_1     A10
#define WS2812_LED_COUNT_1  50
#endif

#ifndef WS2812_PWM_DRIVER_2
#define WS2812_PWM_DRIVER_2 2
#define WS2812_DI_PIN_2     A11
#define WS2812_LED_COUNT_2  4
#endif

#define PWM_Times_1 1

#if WS2812_PWM_DRIVER_1 == 1
#define WS2812_PWM1_CNT_END_REG R32_TMR1_CNT_END
#define WS2812_DMA_CONFIG1(en, start, end)      \
    TMR1_DMACfg(en, (uint16_t)(uint32_t)&start, \
                (uint16_t)(uint32_t)&end, Mode_LOOP)
#define WS2812_PWM_INIT1(level)          TMR1_PWMInit(level, PWM_Times_1);
#define WS2812_PWM_DMA_INTERRUPT_ENABLE1 PFIC_EnableIRQ(TMR1_IRQn);
#define WS2812_PWM_DMA_INTERRUPT_SET1    TMR1_ITCfg(ENABLE, RB_TMR_IE_DMA_END);
#elif WS2812_PWM_DRIVER_1 == 2
#define WS2812_PWM1_CNT_END_REG R32_TMR2_CNT_END
#define WS2812_DMA_CONFIG1(en, start, end)      \
    TMR2_DMACfg(en, (uint16_t)(uint32_t)&start, \
                (uint16_t)(uint32_t)&end, Mode_LOOP)
#define WS2812_PWM_INIT1(level)          TMR2_PWMInit(level, PWM_Times_1);
#define WS2812_PWM_DMA_INTERRUPT_ENABLE1 PFIC_EnableIRQ(TMR2_IRQn);
#define WS2812_PWM_DMA_INTERRUPT_SET1    TMR2_ITCfg(ENABLE, RB_TMR_IE_DMA_END);
#else
#error Unsupported PWM driver for strip1
#endif

#if WS2812_PWM_DRIVER_2 == 1
#define WS2812_PWM2_CNT_END_REG R32_TMR1_CNT_END
#define WS2812_DMA_CONFIG2(en, start, end)      \
    TMR1_DMACfg(en, (uint16_t)(uint32_t)&start, \
                (uint16_t)(uint32_t)&end, Mode_LOOP)
#define WS2812_PWM_INIT2(level)          TMR1_PWMInit(level, PWM_Times_1);
#define WS2812_PWM_DMA_INTERRUPT_ENABLE2 PFIC_EnableIRQ(TMR1_IRQn);
#define WS2812_PWM_DMA_INTERRUPT_SET2    TMR1_ITCfg(ENABLE, RB_TMR_IE_DMA_END);
#elif WS2812_PWM_DRIVER_2 == 2
#define WS2812_PWM2_CNT_END_REG R32_TMR2_CNT_END
#define WS2812_DMA_CONFIG2(en, start, end)      \
    TMR2_DMACfg(en, (uint16_t)(uint32_t)&start, \
                (uint16_t)(uint32_t)&end, Mode_LOOP)
#define WS2812_PWM_INIT2(level)          TMR2_PWMInit(level, PWM_Times_1);
#define WS2812_PWM_DMA_INTERRUPT_ENABLE2 PFIC_EnableIRQ(TMR2_IRQn);
#define WS2812_PWM_DMA_INTERRUPT_SET2    TMR2_ITCfg(ENABLE, RB_TMR_IE_DMA_END);
#else
#error Unsupported PWM driver for strip2
#endif

__INTERRUPT __HIGH_CODE void TMR1_IRQHandler()
{
    R8_TMR1_INTER_EN = 0;
    TMR1_ClearITFlag(RB_TMR_IF_DMA_END);
    do {
        sys_safe_access_enable();
        R8_SLP_CLK_OFF0 |= RB_SLP_CLK_TMR1;
        sys_safe_access_disable();
    } while (!(R8_SLP_CLK_OFF0 & RB_SLP_CLK_TMR1));
}

__INTERRUPT __HIGH_CODE void TMR2_IRQHandler()
{
    R8_TMR2_INTER_EN = 0;
    TMR2_ClearITFlag(RB_TMR_IF_DMA_END);
    do {
        sys_safe_access_enable();
        R8_SLP_CLK_OFF0 |= RB_SLP_CLK_TMR2;
        sys_safe_access_disable();
    } while (!(R8_SLP_CLK_OFF0 & RB_SLP_CLK_TMR2));
}

#ifndef WS2812_PWM_TARGET_PERIOD
#define WS2812_PWM_TARGET_PERIOD 80000
#endif

#define WS2812_PWM_FREQUENCY FREQ_SYS
#define WS2812_PWM_PERIOD    (WS2812_PWM_FREQUENCY / WS2812_PWM_TARGET_PERIOD)

#define WS2812_COLOR_BITS   (WS2812_CHANNELS * 8)
#define WS2812_RESET_BIT_N  (1000 * WS2812_TRST_US / WS2812_TIMING)
#define WS2812_COLOR_BIT_N1 (WS2812_LED_COUNT_1 * WS2812_COLOR_BITS)
#define WS2812_COLOR_BIT_N2 (WS2812_LED_COUNT_2 * WS2812_COLOR_BITS)
#define WS2812_BIT_N1       (WS2812_COLOR_BIT_N1 + WS2812_RESET_BIT_N)
#define WS2812_BIT_N2       (WS2812_COLOR_BIT_N2 + WS2812_RESET_BIT_N)

#define WS2812_DUTYCYCLE_0 (WS2812_PWM_FREQUENCY / (1000000000 / 350))
#define WS2812_DUTYCYCLE_1 (WS2812_PWM_FREQUENCY / (1000000000 / 800))

#define WS2812_BIT_STRIP(led, byte, bit) (WS2812_COLOR_BITS * (led) + 8 * (byte) + (7 - (bit)))

#if (WS2812_BYTE_ORDER == WS2812_BYTE_ORDER_GRB)
#define WS2812_RED_BIT_STRIP(led, bit)   WS2812_BIT_STRIP((led), 1, (bit))
#define WS2812_GREEN_BIT_STRIP(led, bit) WS2812_BIT_STRIP((led), 0, (bit))
#define WS2812_BLUE_BIT_STRIP(led, bit)  WS2812_BIT_STRIP((led), 2, (bit))
#elif (WS2812_BYTE_ORDER == WS2812_BYTE_ORDER_RGB)
#define WS2812_RED_BIT_STRIP(led, bit)   WS2812_BIT_STRIP((led), 0, (bit))
#define WS2812_GREEN_BIT_STRIP(led, bit) WS2812_BIT_STRIP((led), 1, (bit))
#define WS2812_BLUE_BIT_STRIP(led, bit)  WS2812_BIT_STRIP((led), 2, (bit))
#elif (WS2812_BYTE_ORDER == WS2812_BYTE_ORDER_BGR)
#define WS2812_RED_BIT_STRIP(led, bit)   WS2812_BIT_STRIP((led), 2, (bit))
#define WS2812_GREEN_BIT_STRIP(led, bit) WS2812_BIT_STRIP((led), 1, (bit))
#define WS2812_BLUE_BIT_STRIP(led, bit)  WS2812_BIT_STRIP((led), 0, (bit))
#endif

static rgb_led_t ws2812_last_leds1[WS2812_LED_COUNT_1];
static rgb_led_t ws2812_last_leds2[WS2812_LED_COUNT_2];
static uint32_t ws2812_frame_buffer1[WS2812_BIT_N1 + 1];
static uint32_t ws2812_frame_buffer2[WS2812_BIT_N2 + 1];

#ifdef WS2812_DEBUG
static bool ws2812_debug_enabled;
#define ws2812_debug(...)         \
    do {                          \
        if (ws2812_debug_enabled) \
            dprintf(__VA_ARGS__); \
    } while (0)
void ws2812_debug_toggle(void)
{
    ws2812_debug_enabled = !ws2812_debug_enabled;
    dprintf("WS2812 debug %s\n", ws2812_debug_enabled ? "on" : "off");
}
void ws2812_debug_dump_strip1(void)
{
    for (uint16_t i = 0; i < WS2812_LED_COUNT_1; i++) {
        rgb_led_t c = ws2812_last_leds1[i];
        dprintf("strip1[%u]=%u,%u,%u\n", i, c.r, c.g, c.b);
    }
}
#else
#define ws2812_debug(...)
void ws2812_debug_toggle(void)
{
}
void ws2812_debug_dump_strip1(void)
{
}
#endif

static void ws2812_write_led1(uint16_t led_number, uint8_t r, uint8_t g, uint8_t b)
{
#if WS2812_PWM_DRIVER_1 == 1
    do {
        sys_safe_access_enable();
        R8_SLP_CLK_OFF0 &= ~RB_SLP_CLK_TMR1;
        sys_safe_access_disable();
    } while (R8_SLP_CLK_OFF0 & RB_SLP_CLK_TMR1);
#elif WS2812_PWM_DRIVER_1 == 2
    do {
        sys_safe_access_enable();
        R8_SLP_CLK_OFF0 &= ~RB_SLP_CLK_TMR2;
        sys_safe_access_disable();
    } while (R8_SLP_CLK_OFF0 & RB_SLP_CLK_TMR2);
#endif
    ws2812_last_leds1[led_number].r = r;
    ws2812_last_leds1[led_number].g = g;
    ws2812_last_leds1[led_number].b = b;
    ws2812_debug("strip1[%u]=%u,%u,%u\n", led_number, r, g, b);
    for (uint8_t bit = 0; bit < 8; bit++) {
        ws2812_frame_buffer1[WS2812_RED_BIT_STRIP(led_number, bit)] = ((r >> bit) & 0x01) ? WS2812_DUTYCYCLE_1 : WS2812_DUTYCYCLE_0;
        ws2812_frame_buffer1[WS2812_GREEN_BIT_STRIP(led_number, bit)] = ((g >> bit) & 0x01) ? WS2812_DUTYCYCLE_1 : WS2812_DUTYCYCLE_0;
        ws2812_frame_buffer1[WS2812_BLUE_BIT_STRIP(led_number, bit)] = ((b >> bit) & 0x01) ? WS2812_DUTYCYCLE_1 : WS2812_DUTYCYCLE_0;
    }
}

static void ws2812_write_led2(uint16_t led_number, uint8_t r, uint8_t g, uint8_t b)
{
#if WS2812_PWM_DRIVER_2 == 1
    do {
        sys_safe_access_enable();
        R8_SLP_CLK_OFF0 &= ~RB_SLP_CLK_TMR1;
        sys_safe_access_disable();
    } while (R8_SLP_CLK_OFF0 & RB_SLP_CLK_TMR1);
#elif WS2812_PWM_DRIVER_2 == 2
    do {
        sys_safe_access_enable();
        R8_SLP_CLK_OFF0 &= ~RB_SLP_CLK_TMR2;
        sys_safe_access_disable();
    } while (R8_SLP_CLK_OFF0 & RB_SLP_CLK_TMR2);
#endif
    ws2812_last_leds2[led_number].r = r;
    ws2812_last_leds2[led_number].g = g;
    ws2812_last_leds2[led_number].b = b;
    ws2812_debug("strip2[%u]=%u,%u,%u\n", led_number, r, g, b);
    for (uint8_t bit = 0; bit < 8; bit++) {
        ws2812_frame_buffer2[WS2812_RED_BIT_STRIP(led_number, bit)] = ((r >> bit) & 0x01) ? WS2812_DUTYCYCLE_1 : WS2812_DUTYCYCLE_0;
        ws2812_frame_buffer2[WS2812_GREEN_BIT_STRIP(led_number, bit)] = ((g >> bit) & 0x01) ? WS2812_DUTYCYCLE_1 : WS2812_DUTYCYCLE_0;
        ws2812_frame_buffer2[WS2812_BLUE_BIT_STRIP(led_number, bit)] = ((b >> bit) & 0x01) ? WS2812_DUTYCYCLE_1 : WS2812_DUTYCYCLE_0;
    }
}

void ws2812_init(void)
{
    uint32_t i;
    for (i = 0; i < WS2812_COLOR_BIT_N1; i++) {
        ws2812_frame_buffer1[i] = WS2812_DUTYCYCLE_0;
    }
    for (i = 0; i < WS2812_LED_COUNT_1; i++) {
        ws2812_last_leds1[i] = (rgb_led_t){ 0 };
    }
    for (i = 0; i < WS2812_RESET_BIT_N; i++) {
        ws2812_frame_buffer1[i + WS2812_COLOR_BIT_N1] = 0;
    }
    for (i = 0; i < WS2812_COLOR_BIT_N2; i++) {
        ws2812_frame_buffer2[i] = WS2812_DUTYCYCLE_0;
    }
    for (i = 0; i < WS2812_LED_COUNT_2; i++) {
        ws2812_last_leds2[i] = (rgb_led_t){ 0 };
    }
    for (i = 0; i < WS2812_RESET_BIT_N; i++) {
        ws2812_frame_buffer2[i + WS2812_COLOR_BIT_N2] = 0;
    }

    gpio_set_pin_output(WS2812_DI_PIN_1);
    gpio_set_pin_output(WS2812_DI_PIN_2);

    WS2812_PWM1_CNT_END_REG = WS2812_PWM_PERIOD;
    WS2812_DMA_CONFIG1(ENABLE, ws2812_frame_buffer1[0], ws2812_frame_buffer1[WS2812_BIT_N1 + 1]);
    WS2812_PWM_INIT1(High_Level);
    WS2812_PWM_DMA_INTERRUPT_ENABLE1;

    WS2812_PWM2_CNT_END_REG = WS2812_PWM_PERIOD;
    WS2812_DMA_CONFIG2(ENABLE, ws2812_frame_buffer2[0], ws2812_frame_buffer2[WS2812_BIT_N2 + 1]);
    WS2812_PWM_INIT2(High_Level);
    WS2812_PWM_DMA_INTERRUPT_ENABLE2;
}

void ws2812_setleds_pwm1(rgb_led_t *ledarray, uint16_t leds)
{
    if (!ws2812_power_get()) {
        ws2812_power_toggle(true);
    }
    if (leds > WS2812_LED_COUNT_1) {
        leds = WS2812_LED_COUNT_1;
    }
    for (uint16_t i = 0; i < leds; i++) {
        ws2812_write_led1(i, ledarray[i].r, ledarray[i].g, ledarray[i].b);
    }
    WS2812_PWM_DMA_INTERRUPT_SET1;
}

void ws2812_setleds_pwm2(rgb_led_t *ledarray, uint16_t leds)
{
    if (!ws2812_power_get()) {
        ws2812_power_toggle(true);
    }
    if (leds > WS2812_LED_COUNT_2) {
        leds = WS2812_LED_COUNT_2;
    }
    for (uint16_t i = 0; i < leds; i++) {
        ws2812_write_led2(i, ledarray[i].r, ledarray[i].g, ledarray[i].b);
    }
    WS2812_PWM_DMA_INTERRUPT_SET2;
}

void ws2812_setleds(rgb_led_t *ledarray, uint16_t leds)
{
    if (leds == 0) {
        return;
    }
    if (leds <= WS2812_LED_COUNT_1) {
        ws2812_setleds_pwm1(ledarray, leds);
    } else {
        ws2812_setleds_pwm1(ledarray, WS2812_LED_COUNT_1);
        ws2812_setleds_pwm2(&ledarray[WS2812_LED_COUNT_1], leds - WS2812_LED_COUNT_1);
    }
}
