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
#define BOOTMAGIC_ROW    0
#define BOOTMAGIC_COLUMN 0
#define RGBLED_NUM 60
// #define PERMISSIVE_HOLD
#define HOLD_ON_OTHER_KEY_PRESS

#define EARLY_INIT_PERFORM_BOOTLOADER_JUMP FALSE

// 暂时不启用RGB灯光控制
// #define WS2812_EN_PIN   A5
// #define WS2812_EN_LEVEL 1

#define BATTERY_MEASURE_PIN A4
#define POWER_DETECT_PIN    B12

#define BATTERY_INDICATOR_START_INDEX 3
#define BATTERY_INDICATOR_END_INDEX   0

/* define if matrix has ghost */
// #define MATRIX_HAS_GHOST

/* Set 0 if debouncing isn't needed */
#define DEBOUNCE 10

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