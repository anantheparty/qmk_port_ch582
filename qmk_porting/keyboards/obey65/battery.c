/*
 * Battery configuration for Obey65
 * Uses PA4 (ADC CH0) for battery voltage measurement
 * Uses PB12 for power/charging detection
 */

#include "CH58x_common.h"
#include "gpio.h"
#include "debug_uart.h"
#include "qmk_config.h"
#ifdef RGB_MATRIX_ENABLE
#include "rgb_led.h"
#endif

// Custom battery voltage map for Obey65 hardware
// ADC values (x10) for 0-100% at 3.0V-4.2V range with voltage divider
// Assuming 1:1 voltage divider (Vbat/2), ADC ref 3.3V, 12-bit resolution
// ADC = (Vbat/2) / 3.3V * 4096 * 10
// 3.0V -> (1.5/3.3)*4096*10 = 18618
// 4.2V -> (2.1/3.3)*4096*10 = 26067

const uint16_t battery_map[] = {
    // Customized for Obey65 with 1:1 voltage divider
    // These values need calibration with actual hardware
    18618, 18864, 19110, 19356, 19602, // 0-4%   3.00-3.08V
    19848, 20094, 20340, 20586, 20832, // 5-9%   3.10-3.18V
    21078, 21324, 21570, 21816, 22062, // 10-14% 3.20-3.28V
    22308, 22554, 22800, 23046, 23292, // 15-19% 3.30-3.38V
    23538, 23661, 23784, 23907, 24030, // 20-24% 3.40-3.46V
    24153, 24276, 24399, 24522, 24645, // 25-29% 3.48-3.54V
    24768, 24891, 25014, 25137, 25260, // 30-34% 3.56-3.62V
    25383, 25506, 25629, 25752, 25875, // 35-39% 3.64-3.70V
    25998, 26067, 26067, 26067, 26067, // 40-44% 3.72-3.76V
    26067, 26067, 26067, 26067, 26067, // 45-49% 3.78-3.82V
    26067, 26067, 26067, 26067, 26067, // 50-54% 3.84-3.88V
    26067, 26067, 26067, 26067, 26067, // 55-59% 3.90-3.94V
    26067, 26067, 26067, 26067, 26067, // 60-64% 3.96-4.00V
    26067, 26067, 26067, 26067, 26067, // 65-69% 4.02-4.06V
    26067, 26067, 26067, 26067, 26067, // 70-74% 4.08-4.12V
    26067, 26067, 26067, 26067, 26067, // 75-79% 4.14-4.18V
    26067, 26067, 26067, 26067, 26067, // 80-84% 4.18-4.20V
    26067, 26067, 26067, 26067, 26067, // 85-89%
    26067, 26067, 26067, 26067, 26067, // 90-94%
    26067, 26067, 26067, 26067, 26067  // 95-100%
};

// Required by platform battery code - configure GPIOs before deep sleep
void battery_critical_gpio_prerequisite(void) {
    // Configure all non-essential GPIOs to low power state
    // Keep matrix pins in a safe state
    // This is called before entering deep sleep due to critical battery

    DEBUG_PRINTF("[BAT] Critical! Preparing for shutdown\r\n");

    // Disable RGB LEDs
#ifdef RGB_MATRIX_ENABLE
    rgbled_power_off();
#endif

    // Configure POWER_DETECT_PIN for wake-up
#ifdef POWER_DETECT_PIN
    gpio_set_pin_input_low(POWER_DETECT_PIN);
#endif
}

// Battery status for obey65
typedef struct {
    uint8_t percent;
    uint16_t voltage_mv;
    bool charging;
    bool usb_connected;
} battery_status_t;

static battery_status_t bat_status = {
    .percent = 100,
    .voltage_mv = 4200,
    .charging = false,
    .usb_connected = false
};

// Update battery status (call periodically)
void obey65_battery_update(void) {
    extern uint16_t battery_measure(void);
    extern uint8_t battery_calculate(uint16_t adcVal);

    uint16_t adc_val = battery_measure();
    bat_status.percent = battery_calculate(adc_val);

    // Calculate approximate voltage (mV) from ADC
    // With 1:1 divider: Vbat = ADC / 4096 * 3.3V * 2 * 1000
    bat_status.voltage_mv = (uint32_t)adc_val * 3300 * 2 / 4096 / 10;

    // Check charging status
#ifdef POWER_DETECT_PIN
    bat_status.usb_connected = gpio_read_pin(POWER_DETECT_PIN);
    // Note: actual charging detection may need additional logic
    // based on charge IC status pin
    bat_status.charging = bat_status.usb_connected && (bat_status.percent < 100);
#endif

    DEBUG_PRINTF("[BAT] %d%% %dmV %s\r\n",
                 bat_status.percent,
                 bat_status.voltage_mv,
                 bat_status.charging ? "CHG" : (bat_status.usb_connected ? "USB" : "BAT"));
}

// Get battery percentage
uint8_t obey65_battery_get_percent(void) {
    return bat_status.percent;
}

// Get battery voltage in mV
uint16_t obey65_battery_get_voltage(void) {
    return bat_status.voltage_mv;
}

// Check if charging
bool obey65_battery_is_charging(void) {
    return bat_status.charging;
}

// Check if USB connected
bool obey65_battery_is_usb_connected(void) {
    return bat_status.usb_connected;
}
