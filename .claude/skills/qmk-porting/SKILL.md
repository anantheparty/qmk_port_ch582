---
name: qmk-porting
description: QMK 固件移植和键盘定义指南。当任务涉及创建新键盘、修改 keymap、配置 QMK 功能、VIA 支持、RGB 灯效时使用。
---

# QMK 移植开发指南

## 键盘定义结构

### 最小文件集
```
keyboards/my_keyboard/
├── config.h          # 硬件配置
├── halconf.h         # HAL 配置
├── mcuconf.h         # MCU 配置  
├── my_keyboard.c     # 键盘初始化代码
├── my_keyboard.h     # 布局宏定义
└── keymaps/
    └── default/
        └── keymap.c  # 键位映射
```

### config.h 模板
```c
#pragma once

// 矩阵配置
#define MATRIX_ROWS 5
#define MATRIX_COLS 15
#define MATRIX_ROW_PINS { A4, A5, A6, A7, A8 }
#define MATRIX_COL_PINS { B4, B5, B6, B7, B10, B11, B12, B13, B14, B15, B20, B21, B22, B23, A15 }
#define DIODE_DIRECTION COL2ROW

// USB 配置
#define VENDOR_ID    0xFEED
#define PRODUCT_ID   0x0001
#define DEVICE_VER   0x0001
#define MANUFACTURER "MyName"
#define PRODUCT      "MyKeyboard"

// 功能开关
#define TAPPING_TERM 200
#define PERMISSIVE_HOLD

// RGB 配置 (如需要)
#define RGB_MATRIX_LED_COUNT 75
#define WS2812_SPI_DRIVER SPID1
#define WS2812_SPI_MOSI_PIN B0
```

### halconf.h 模板
```c
#pragma once

#define HAL_USE_SPI TRUE  // WS2812 需要
#define HAL_USE_I2C TRUE  // OLED/传感器需要

#include_next <halconf.h>
```

## 键位映射

### keymap.c 模板
```c
#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_ENT,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT,
        KC_LCTL, KC_LGUI, KC_LALT,                   KC_SPC,                    KC_RALT, MO(1),   KC_RGUI, KC_RCTL
    ),
    [1] = LAYOUT(
        KC_GRV,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_DEL,
        _______, RGB_TOG, RGB_MOD, RGB_HUI, RGB_SAI, RGB_VAI, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, RGB_M_P, RGB_M_B, RGB_HUD, RGB_SAD, RGB_VAD, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______,                   _______,                   _______, _______, _______, QK_BOOT
    )
};
```

## VIA 支持

### 启用 VIA
在 config.h 或 rules.mk 中:
```c
#define VIA_ENABLE
#define DYNAMIC_KEYMAP_LAYER_COUNT 4
```

### via.json 结构
```json
{
  "name": "My Keyboard",
  "vendorId": "0xFEED",
  "productId": "0x0001",
  "matrix": {"rows": 5, "cols": 15},
  "layouts": {
    "keymap": [
      // 定义布局...
    ]
  }
}
```

## RGB Matrix

### WS2812 配置 (SPI 模式)
```c
#define RGB_MATRIX_LED_COUNT 75
#define WS2812_SPI_DRIVER SPID1
#define WS2812_SPI_MOSI_PIN B0
#define WS2812_SPI_SCK_PIN NO_PIN
#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 200
```

### 灯位定义
```c
led_config_t g_led_config = {
    // 矩阵位置映射
    {
        {0,  1,  2,  3,  4},
        {5,  6,  7,  8,  9},
        // ...
    },
    // 物理位置 (x, y)
    {
        {0, 0}, {16, 0}, {32, 0}, {48, 0}, {64, 0},
        // ...
    },
    // 灯类型 (1=矩阵, 2=指示灯, 4=底灯)
    {
        1, 1, 1, 1, 1,
        // ...
    }
};
```

## CMake 集成

### 添加新键盘到构建系统
在 `qmk_porting/keyboards/CMakeLists.txt` 中:
```cmake
if(KEYBOARD STREQUAL "my_keyboard")
    set(KEYBOARD_PATH "${CMAKE_CURRENT_LIST_DIR}/my_keyboard")
    include_directories(${KEYBOARD_PATH})
    list(APPEND KEYBOARD_SOURCES
        "${KEYBOARD_PATH}/my_keyboard.c"
    )
endif()
```

## 常见问题

### 编译错误: multiple definition of config.h
**解决**: 使用完整路径包含头文件
```c
// 错误
#include "config.h"
// 正确
#include "keyboards/my_keyboard/config.h"
```

### VIA 无法识别键盘
检查:
1. `VENDOR_ID` 和 `PRODUCT_ID` 是否匹配
2. via.json 是否正确上传到 VIA
3. USB 描述符是否正确
