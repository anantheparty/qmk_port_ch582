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
#define RGBLED_NUM 4
// #define PERMISSIVE_HOLD
#define HOLD_ON_OTHER_KEY_PRESS

// 禁用自动DFU模式，让你有机会手动拖入uf2文件
#define EARLY_INIT_PERFORM_BOOTLOADER_JUMP FALSE

// 启用简单的bootloader
#define BOOTLOADER_ENABLE
#define BOOTLOADER_SIZE 0x6000

// Bootloader配置说明：
// 1. 首次使用：使用 obey65_factory_*.hex 通过串口刷写
// 2. 后续升级：使用 obey65_upgrade_*.uf2 通过UF2方式
// 3. 进入bootloader：按住ESC键插入USB，或使用 KC_BOOTLOADER_JUMP 按键

#define WS2812_EN_PIN   A11
#define WS2812_EN_LEVEL 1

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
#define DEBOUNCE 0

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