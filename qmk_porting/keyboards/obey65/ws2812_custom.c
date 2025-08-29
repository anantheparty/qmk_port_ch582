#include "ws2812_custom.h"
#include "ws2812.h"
#include "quantum.h"

// Externs from PWM driver
extern void ws2812_setleds_pwm1(rgb_led_t *ledarray, uint16_t leds);
extern void ws2812_setleds_pwm2(rgb_led_t *ledarray, uint16_t leds);

#define MAX_LIGHT 35

ws2812_color_t current_color_4led = {0, 0, 0};
ws2812_color_t current_color_50led = {0, 0, 0};
bool ws2812_power_4led = true;
bool ws2812_power_50led = true;

rgb_control_t rgb_control_4led = {4, 4, 4, 20};
rgb_control_t rgb_control_50led = {4, 4, 4, 20};

static void send_strip_colors(led_strip_type_t strip, uint8_t r, uint8_t g, uint8_t b) {
    if (strip == LED_STRIP_4) {
        rgb_led_t leds[WS2812_LED_COUNT_2];
        for (uint8_t i = 0; i < WS2812_LED_COUNT_2; i++) {
            leds[i].r = r;
            leds[i].g = g;
            leds[i].b = b;
        }
        ws2812_setleds_pwm2(leds, WS2812_LED_COUNT_2);
    } else {
        rgb_led_t leds[WS2812_LED_COUNT_1];
        for (uint8_t i = 0; i < WS2812_LED_COUNT_1; i++) {
            leds[i].r = r;
            leds[i].g = g;
            leds[i].b = b;
        }
        ws2812_setleds_pwm1(leds, WS2812_LED_COUNT_1);
    }
}

void ws2812_custom_init(void) {
    ws2812_init();
    ws2812_power_4led = true;
    ws2812_power_50led = true;
    ws2812_custom_update_led_strip(LED_STRIP_4);
    ws2812_custom_update_led_strip(LED_STRIP_50);
}

void ws2812_custom_send_96bits(void) {
    ws2812_custom_send_strip(LED_STRIP_4);
    ws2812_custom_send_strip(LED_STRIP_50);
}

void ws2812_custom_send_strip(led_strip_type_t strip) {
    if (strip == LED_STRIP_4) {
        uint8_t r = ws2812_power_4led ? current_color_4led.r : 0;
        uint8_t g = ws2812_power_4led ? current_color_4led.g : 0;
        uint8_t b = ws2812_power_4led ? current_color_4led.b : 0;
        send_strip_colors(strip, r, g, b);
    } else {
        uint8_t r = ws2812_power_50led ? current_color_50led.r : 0;
        uint8_t g = ws2812_power_50led ? current_color_50led.g : 0;
        uint8_t b = ws2812_power_50led ? current_color_50led.b : 0;
        send_strip_colors(strip, r, g, b);
    }
}

void ws2812_custom_adjust_rgb_strip(uint8_t channel, int8_t delta, led_strip_type_t strip) {
    rgb_control_t *control = (strip == LED_STRIP_4) ? &rgb_control_4led : &rgb_control_50led;

    switch (channel) {
        case 0:
            control->r += delta;
            if (control->r > 200) control->r = 0;
            if (control->r > 10) control->r = 10;
            break;
        case 1:
            control->g += delta;
            if (control->g > 200) control->g = 0;
            if (control->g > 10) control->g = 10;
            break;
        case 2:
            control->b += delta;
            if (control->b > 200) control->b = 0;
            if (control->b > 10) control->b = 10;
            break;
    }
    ws2812_custom_update_led_strip(strip);
}

void ws2812_custom_adjust_brightness_strip(int8_t delta, led_strip_type_t strip) {
    rgb_control_t *control = (strip == LED_STRIP_4) ? &rgb_control_4led : &rgb_control_50led;
    int16_t new_brightness = control->brightness + delta;
    if (new_brightness < 0) new_brightness = 0;
    if (new_brightness > 100) new_brightness = 100;
    control->brightness = (uint8_t)new_brightness;
    ws2812_custom_update_led_strip(strip);
}

rgb_control_t ws2812_custom_get_rgb_control_strip(led_strip_type_t strip) {
    return (strip == LED_STRIP_4) ? rgb_control_4led : rgb_control_50led;
}

void ws2812_custom_update_led_strip(led_strip_type_t strip) {
    rgb_control_t *control = (strip == LED_STRIP_4) ? &rgb_control_4led : &rgb_control_50led;
    ws2812_color_t *color = (strip == LED_STRIP_4) ? &current_color_4led : &current_color_50led;
    float brightness_factor = control->brightness / 100.0f;
    color->r = (uint8_t)(control->r * brightness_factor * MAX_LIGHT / 10);
    color->g = (uint8_t)(control->g * brightness_factor * MAX_LIGHT / 10);
    color->b = (uint8_t)(control->b * brightness_factor * MAX_LIGHT / 10);
    ws2812_custom_send_strip(strip);
}

void ws2812_custom_set_color_temp(uint8_t r, uint8_t g, uint8_t b) {
    r %= MAX_LIGHT;
    g %= MAX_LIGHT;
    b %= MAX_LIGHT;
    send_strip_colors(LED_STRIP_4, r, g, b);
    send_strip_colors(LED_STRIP_50, r, g, b);
}

void ws2812_toggle_power(bool power) {
    ws2812_power_4led = power;
    ws2812_power_50led = power;
    ws2812_custom_send_strip(LED_STRIP_4);
    ws2812_custom_send_strip(LED_STRIP_50);
}

bool ws2812_custom_power_get(void) {
    return ws2812_power_4led && ws2812_power_50led;
}

void ws2812_custom_adjust_color(uint8_t channel, int8_t delta) {
    ws2812_custom_adjust_color_strip(channel, delta, LED_STRIP_4);
    ws2812_custom_adjust_color_strip(channel, delta, LED_STRIP_50);
}

void ws2812_custom_adjust_color_strip(uint8_t channel, int8_t delta, led_strip_type_t strip) {
    ws2812_color_t *current_color = (strip == LED_STRIP_4) ? &current_color_4led : &current_color_50led;
    uint8_t *component = NULL;
    switch (channel) {
        case 0: component = &current_color->r; break;
        case 1: component = &current_color->g; break;
        case 2: component = &current_color->b; break;
        default: return;
    }
    uint8_t new_value = *component + delta + MAX_LIGHT;
    new_value %= MAX_LIGHT;
    *component = new_value;
    ws2812_custom_send_strip(strip);
}

ws2812_color_t ws2812_custom_get_current_color_strip(led_strip_type_t strip) {
    return (strip == LED_STRIP_4) ? current_color_4led : current_color_50led;
}

void ws2812_custom_send_off_signal(led_strip_type_t strip) {
    send_strip_colors(strip, 0, 0, 0);
}

void ws2812_custom_send_on_signal(led_strip_type_t strip) {
    ws2812_custom_send_strip(strip);
}

