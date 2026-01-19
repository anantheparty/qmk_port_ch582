/**
 * BLE 支持代码 - 提供 BLE 必需的变量定义和桩函数
 *
 * 此文件为 BLE 功能提供：
 * 1. MEM_BUF - BLE 协议栈内存缓冲区
 * 2. MacAddr - 蓝牙 MAC 地址
 * 3. 无线库接口桩函数
 */

#include "config.h"
#include "quantum.h"
#include "protocol.h"

// ============================================================================
// BLE 内存和 MAC 地址定义
// ============================================================================

// BLE 协议栈使用的内存缓冲区
// 大小由 BLE_MEMHEAP_SIZE 定义 (默认 6KB)
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

// 蓝牙 MAC 地址 (使用芯片内置 MAC)
const uint8_t MacAddr[6] = {0};

// ============================================================================
// 无线库接口桩函数
// 这些函数在 qmk_porting 代码中被调用，需要提供实现或桩函数
// ============================================================================

/**
 * 无线指示器状态重置
 * 在键盘关机时调用
 */
void wireless_indicator_status_reset(void) {
    // TODO: 实现无线状态指示重置
}

/**
 * 无线指示器守护进程
 * 在主循环中周期性调用
 */
void wireless_indicator_daemon(void) {
    // TODO: 实现无线状态指示更新
}

/**
 * RGB 无线指示任务
 * 在 RGB Matrix 指示回调中调用
 */
void wireless_rgb_indicator_task(uint8_t led_min, uint8_t led_max) {
    // TODO: 实现 RGB LED 无线状态指示
    // 例如：显示电池电量、连接状态等
}

/**
 * 无线按键预处理
 * 在 pre_process_record_kb 中调用
 * @return true 继续处理，false 停止处理
 */
bool wireless_pre_process_record_kb(uint16_t keycode, keyrecord_t *record) {
    // TODO: 实现无线相关按键预处理
    return true;  // 继续处理
}

/**
 * 无线按键处理
 * 处理无线相关的按键码（BLE_SLOT、ESB_MODE 等）
 * @return true 已处理，false 未处理
 */
bool wireless_process_record(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return false;
    }

    // TODO: 实现无线相关按键处理
    // BLE_SLOT0..BLE_SLOT3: 切换 BLE 配对槽位
    // BLE_ALL_CLEAR: 清除所有 BLE 配对
    // ESB_MODE: 切换到 2.4G 模式
    // BATTERY_INDICATOR: 显示电池电量

    return false;
}

/**
 * BLE 密码处理
 * 处理 BLE 配对时的密码输入
 * @return true 已处理，false 未处理
 */
bool process_ble_passcode(uint16_t keycode, keyrecord_t *record) {
    // TODO: 实现 BLE 密码输入处理
    return false;
}

/**
 * BLE 密码键盘回调
 * @return true 已处理，false 未处理
 */
bool process_ble_passcode_kb(uint16_t keycode, keyrecord_t *record) {
    return process_ble_passcode(keycode, record);
}
