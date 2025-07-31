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

#pragma once

/* USB Device descriptor parameter */
#define VENDOR_ID    0xCAFE
#define PRODUCT_ID   0x0B97
#define DEVICE_VER   0x0001
#define MANUFACTURER Obey65
#define PRODUCT      Obey65

#define MATRIX_ROWS 5
#define MATRIX_COLS 15
#define MATRIX_ROW_PINS      \
    {                        \
        A8, A9, B9, B13, A7  \
    }
#define MATRIX_COL_PINS                                                          \
    {                                                                            \
        B8, B16, B15, B14, B7, B6, B5, B4, B3, B2, B1, B0, B21, B20, B19       \
    }
#define DYNAMIC_KEYMAP_LAYER_COUNT 10

#define DIODE_DIRECTION  COL2ROW
// 使用ESC键作为BOOTMAGIC触发键（第0行第0列）
#define BOOTMAGIC_ROW    0
#define BOOTMAGIC_COLUMN 0

// PWM 驱动定义
#define WS2812_PWM_DRIVER 2         // 使用 TMR2（PA11）
#define WS2812_DI_PIN     A11       // 接在 PA11
#define WS2812_DRIVER     pwm       // 使用 PWM 驱动

// WS2812 使能引脚配置
#define WS2812_EN_PIN     A11       // 使用 PA10 作为使能引脚（TMR1 PWM输出）
#define WS2812_EN_LEVEL   1         // 高电平使能
// #define WS2812_PWM_TARGET_PERIOD 800000 // PWM周期 = 1.25us，对应800kHz
#define WS2812_TIMING 1250
#define WS2812_T1H 850
#define WS2812_T0H 250
#define WS2812_TRST_US 100

#define WS2812_BYTE_ORDER WS2812_BYTE_ORDER_GRB  // 可选，根据颜色实际效果调整

// RGB Matrix 设置
#define RGBLED_NUM                    4
#define RGB_MATRIX_LED_COUNT          RGBLED_NUM
#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 32
#define RGB_MATRIX_STARTUP_VAL        RGB_MATRIX_MAXIMUM_BRIGHTNESS
#define RGB_MATRIX_HUE_STEP           10
#define RGB_MATRIX_SAT_STEP           8
#define RGB_MATRIX_VAL_STEP           4
#define RGB_MATRIX_SPD_STEP           10
#define RGB_MATRIX_SLEEP

#ifdef RGB_MATRIX_ENABLE
// 灯效（可按需精简）
#define ENABLE_RGB_MATRIX_ALPHAS_MODS
#define ENABLE_RGB_MATRIX_GRADIENT_UP_DOWN
#define ENABLE_RGB_MATRIX_GRADIENT_LEFT_RIGHT
#define ENABLE_RGB_MATRIX_BREATHING
#define ENABLE_RGB_MATRIX_BAND_SAT
#define ENABLE_RGB_MATRIX_BAND_VAL
#define ENABLE_RGB_MATRIX_BAND_PINWHEEL_SAT
#define ENABLE_RGB_MATRIX_BAND_PINWHEEL_VAL
#define ENABLE_RGB_MATRIX_BAND_SPIRAL_SAT
#define ENABLE_RGB_MATRIX_BAND_SPIRAL_VAL
#define ENABLE_RGB_MATRIX_CYCLE_ALL
#define ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
#define ENABLE_RGB_MATRIX_CYCLE_UP_DOWN
#define ENABLE_RGB_MATRIX_RAINBOW_MOVING_CHEVRON
#define ENABLE_RGB_MATRIX_CYCLE_OUT_IN
#define ENABLE_RGB_MATRIX_CYCLE_OUT_IN_DUAL
#define ENABLE_RGB_MATRIX_CYCLE_PINWHEEL
#define ENABLE_RGB_MATRIX_CYCLE_SPIRAL
#define ENABLE_RGB_MATRIX_DUAL_BEACON
#define ENABLE_RGB_MATRIX_RAINBOW_BEACON
#define ENABLE_RGB_MATRIX_RAINBOW_PINWHEELS
#define ENABLE_RGB_MATRIX_RAINDROPS
#define ENABLE_RGB_MATRIX_JELLYBEAN_RAINDROPS
#define ENABLE_RGB_MATRIX_HUE_BREATHING
#define ENABLE_RGB_MATRIX_HUE_PENDULUM
#define ENABLE_RGB_MATRIX_HUE_WAVE
#define ENABLE_RGB_MATRIX_TYPING_HEATMAP
#define ENABLE_RGB_MATRIX_DIGITAL_RAIN
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_WIDE
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTIWIDE
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_CROSS
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTICROSS
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_NEXUS
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTINEXUS
#define ENABLE_RGB_MATRIX_SPLASH
#define ENABLE_RGB_MATRIX_MULTISPLASH
#define ENABLE_RGB_MATRIX_SOLID_SPLASH
#define ENABLE_RGB_MATRIX_SOLID_MULTISPLASH
#define ENABLE_RGB_MATRIX_PIXEL_RAIN
#define ENABLE_RGB_MATRIX_PIXEL_FLOW
#define ENABLE_RGB_MATRIX_PIXEL_FRACTAL
#endif
#ifdef RGB_RAW_ENABLE
#define AUXILIARY_RGB_USE_UNIVERSAL_BRIGHTNESS
#endif

// #define PERMISSIVE_HOLD
#define HOLD_ON_OTHER_KEY_PRESS

// NKRO配置 - 解决只能同时输入6个字符的问题
#define NKRO_ENABLE
#define FORCE_NKRO

// 禁用自动DFU模式，让你有机会手动拖入uf2文件
#define EARLY_INIT_PERFORM_BOOTLOADER_JUMP FALSE

// 启用简单的bootloader
#define BOOTLOADER_ENABLE
#define BOOTLOADER_SIZE 0x6000

// Bootloader配置说明：
// 1. 首次使用：使用 obey65_factory_*.hex 通过串口刷写
// 2. 后续升级：使用 obey65_upgrade_*.uf2 通过UF2方式
// 3. 进入bootloader：按住ESC键插入USB，或使用 KC_BOOTLOADER_JUMP 按键

#define BATTERY_MEASURE_PIN A4
#define POWER_DETECT_PIN    B12

#define BATTERY_INDICATOR_START_INDEX 3
#define BATTERY_INDICATOR_END_INDEX   0

// 大写锁定LED引脚定义
#define LED_CAPS_LOCK_PIN B17
#define LED_PIN_ON_STATE 1

/* define if matrix has ghost */
// #define MATRIX_HAS_GHOST

/* Set 0 if debouncing isn't needed */
#define DEBOUNCE 1
/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

/* disable debug print */
// #define NO_DEBUG

/* disable print */
// #define NO_PRINT

/* disable action features */
// #define NO_ACTION_LAYER
// #define NO_ACTION_TAPPING
// #define NO_ACTION_ONESHOT 