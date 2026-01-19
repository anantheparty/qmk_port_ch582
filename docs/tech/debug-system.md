# 键盘输入调试系统设计

## 1. 背景

Obey65 键盘 PCB 没有预留串口调试引脚，传统的 printf 调试无法使用。需要设计一套通过键盘输入"打字"输出调试信息的系统。

## 2. 设计原则

1. **非侵入性**: 不影响正常键盘使用
2. **易于触发**: 使用组合键进入调试模式
3. **信息完整**: 能输出足够的调试信息
4. **可扩展**: 方便添加新的调试命令

## 3. 调试键组合定义

### 3.1 进入调试模式

```
Fn + D  -> 进入调试模式 (持续2秒内有效)
```

### 3.2 调试命令

进入调试模式后，按数字键输出对应信息：

| 按键 | 调试信息 |
|------|----------|
| 1 | 当前连接模式 (USB/BLE/2.4G) |
| 2 | 电池电量百分比 |
| 3 | 蓝牙连接状态 |
| 4 | 2.4G 连接状态 |
| 5 | RGB 状态 (开/关/模式) |
| 6 | 固件版本 |
| 7 | 运行时间 (uptime) |
| 8 | EEPROM 状态 |
| 9 | 内存使用情况 |
| 0 | 完整状态报告 |

### 3.3 退出调试模式

- 按 `ESC` 键退出
- 或者 2 秒无操作自动退出

## 4. 实现方案

### 4.1 数据结构

```c
// debug_output.h

#pragma once

#include <stdint.h>
#include <stdbool.h>

// 调试模式状态
typedef enum {
    DEBUG_MODE_OFF,
    DEBUG_MODE_WAITING,  // 等待命令输入
    DEBUG_MODE_OUTPUTTING  // 正在输出
} debug_mode_t;

// 调试命令
typedef enum {
    DEBUG_CMD_MODE = 1,      // 当前模式
    DEBUG_CMD_BATTERY = 2,   // 电池电量
    DEBUG_CMD_BLE_STATUS = 3,// 蓝牙状态
    DEBUG_CMD_ESB_STATUS = 4,// 2.4G状态
    DEBUG_CMD_RGB_STATUS = 5,// RGB状态
    DEBUG_CMD_VERSION = 6,   // 固件版本
    DEBUG_CMD_UPTIME = 7,    // 运行时间
    DEBUG_CMD_EEPROM = 8,    // EEPROM状态
    DEBUG_CMD_MEMORY = 9,    // 内存使用
    DEBUG_CMD_FULL = 0,      // 完整报告
} debug_cmd_t;

// 初始化调试系统
void debug_output_init(void);

// 处理调试按键（在 process_record_kb 中调用）
bool debug_process_key(uint16_t keycode, keyrecord_t *record);

// 调试模式任务（在主循环中调用）
void debug_output_task(void);

// 检查是否在调试模式
bool is_debug_mode_active(void);
```

### 4.2 核心实现

```c
// debug_output.c

#include "debug_output.h"
#include "quantum.h"
#include "timer.h"

#define DEBUG_TIMEOUT_MS 2000
#define DEBUG_OUTPUT_DELAY_MS 10  // 字符间延迟

static debug_mode_t debug_mode = DEBUG_MODE_OFF;
static uint32_t debug_mode_timer = 0;
static char output_buffer[256];
static uint16_t output_index = 0;
static uint16_t output_length = 0;
static uint32_t last_output_time = 0;

// Fn + D 触发调试模式
static bool fn_pressed = false;

bool debug_process_key(uint16_t keycode, keyrecord_t *record) {
    // 检测 Fn 键状态
    if (keycode == MO(1)) {  // 假设 Fn 是 layer 1
        fn_pressed = record->event.pressed;
        return true;
    }

    // Fn + D 进入调试模式
    if (fn_pressed && keycode == KC_D && record->event.pressed) {
        debug_mode = DEBUG_MODE_WAITING;
        debug_mode_timer = timer_read32();
        return false;  // 阻止 D 键输出
    }

    // 调试模式下处理命令
    if (debug_mode == DEBUG_MODE_WAITING && record->event.pressed) {
        if (keycode == KC_ESC) {
            debug_mode = DEBUG_MODE_OFF;
            return false;
        }

        if (keycode >= KC_1 && keycode <= KC_0) {
            uint8_t cmd = (keycode == KC_0) ? 0 : (keycode - KC_1 + 1);
            debug_execute_command(cmd);
            return false;
        }
    }

    return true;
}

static void debug_execute_command(uint8_t cmd) {
    output_index = 0;
    output_length = 0;

    switch (cmd) {
        case DEBUG_CMD_MODE:
            output_length = snprintf(output_buffer, sizeof(output_buffer),
                "[Mode: %s] ",
                kbd_protocol_type == kbd_protocol_usb ? "USB" :
                kbd_protocol_type == kbd_protocol_ble ? "BLE" : "2.4G");
            break;

        case DEBUG_CMD_BATTERY:
            output_length = snprintf(output_buffer, sizeof(output_buffer),
                "[Battery: %d%%] ", get_battery_percent());
            break;

        case DEBUG_CMD_BLE_STATUS:
            output_length = snprintf(output_buffer, sizeof(output_buffer),
                "[BLE: %s, Slot: %d] ",
                is_ble_connected() ? "Connected" : "Disconnected",
                get_ble_slot());
            break;

        case DEBUG_CMD_VERSION:
            output_length = snprintf(output_buffer, sizeof(output_buffer),
                "[FW: %s, Build: %s] ",
                MACRO2STR(__GIT_VERSION__), QMK_BUILDDATE);
            break;

        case DEBUG_CMD_UPTIME:
            {
                uint32_t uptime_ms = timer_read32();
                uint32_t seconds = uptime_ms / 1000;
                uint32_t minutes = seconds / 60;
                uint32_t hours = minutes / 60;
                output_length = snprintf(output_buffer, sizeof(output_buffer),
                    "[Uptime: %luh%lum%lus] ",
                    hours, minutes % 60, seconds % 60);
            }
            break;

        // ... 其他命令

        default:
            output_length = snprintf(output_buffer, sizeof(output_buffer),
                "[Unknown command] ");
            break;
    }

    if (output_length > 0) {
        debug_mode = DEBUG_MODE_OUTPUTTING;
        last_output_time = timer_read32();
    }
}

void debug_output_task(void) {
    // 超时检查
    if (debug_mode == DEBUG_MODE_WAITING) {
        if (timer_elapsed32(debug_mode_timer) > DEBUG_TIMEOUT_MS) {
            debug_mode = DEBUG_MODE_OFF;
        }
        return;
    }

    // 输出字符
    if (debug_mode == DEBUG_MODE_OUTPUTTING) {
        if (timer_elapsed32(last_output_time) >= DEBUG_OUTPUT_DELAY_MS) {
            if (output_index < output_length) {
                char c = output_buffer[output_index++];
                debug_send_char(c);
                last_output_time = timer_read32();
            } else {
                debug_mode = DEBUG_MODE_OFF;
            }
        }
    }
}

static void debug_send_char(char c) {
    uint8_t keycode = 0;
    bool shift = false;

    // 将字符转换为按键码
    if (c >= 'a' && c <= 'z') {
        keycode = KC_A + (c - 'a');
    } else if (c >= 'A' && c <= 'Z') {
        keycode = KC_A + (c - 'A');
        shift = true;
    } else if (c >= '0' && c <= '9') {
        keycode = (c == '0') ? KC_0 : KC_1 + (c - '1');
    } else {
        switch (c) {
            case ' ': keycode = KC_SPACE; break;
            case ':': keycode = KC_SCLN; shift = true; break;
            case '[': keycode = KC_LBRC; break;
            case ']': keycode = KC_RBRC; break;
            case '%': keycode = KC_5; shift = true; break;
            case ',': keycode = KC_COMM; break;
            default: keycode = KC_SPACE; break;
        }
    }

    // 发送按键
    if (keycode) {
        if (shift) {
            register_code(KC_LSFT);
        }
        tap_code(keycode);
        if (shift) {
            unregister_code(KC_LSFT);
        }
    }
}

bool is_debug_mode_active(void) {
    return debug_mode != DEBUG_MODE_OFF;
}
```

### 4.3 集成到键盘

```c
// obey65.c 中添加

#include "debug_output.h"

void keyboard_post_init_kb(void) {
    debug_output_init();
    keyboard_post_init_user();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    // 先处理调试按键
    if (!debug_process_key(keycode, record)) {
        return false;
    }

    return process_record_user(keycode, record);
}

void housekeeping_task_kb(void) {
    debug_output_task();
    housekeeping_task_user();
}
```

## 5. 使用示例

1. 按住 `Fn` + `D` 进入调试模式
2. 按 `2` 查看电池电量
3. 键盘会"打出": `[Battery: 75%]`
4. 按 `ESC` 退出调试模式

## 6. 扩展建议

### 6.1 添加新的调试命令

在 `debug_execute_command` 中添加新的 case：

```c
case DEBUG_CMD_CUSTOM:
    output_length = snprintf(output_buffer, sizeof(output_buffer),
        "[Your custom debug info]");
    break;
```

### 6.2 增加调试级别

可以用 `Fn + D + D` 进入详细调试模式，输出更多信息。

### 6.3 添加 LED 指示

进入调试模式时可以让 RGB 闪烁，提示用户当前处于调试状态。

---

*文档创建日期: 2026-01-19*
