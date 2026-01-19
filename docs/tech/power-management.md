# 电源管理方案

## 1. 概述

本文档描述 Obey65 键盘的电源管理设计，包括电池检测、充电管理和低功耗策略。

## 2. 硬件配置

### 2.1 电池相关引脚

| 功能 | 引脚 | 说明 |
|------|------|------|
| 电池电压检测 | PA4 (BATTERY_MEASURE_PIN) | ADC 输入 |
| 充电检测 | PB12 (POWER_DETECT_PIN) | 高电平=充电中 |

### 2.2 电路说明

```
                    ┌─────────────┐
   VBAT ────────────┤             │
    │               │   充电IC    │──── USB_VBUS
    │               │   (TP4054)  │
    │               └─────────────┘
    │                      │
    │                      │ CHRG (充电指示)
    │                      ▼
    │               ┌─────────────┐
    │               │    PB12     │ (POWER_DETECT_PIN)
    │               └─────────────┘
    │
    ├───[R1 100K]───┬───[R2 100K]───GND
    │               │
    │               ▼
    │        ┌─────────────┐
    │        │    PA4      │ (BATTERY_MEASURE_PIN)
    │        │   ADC_CH4   │
    │        └─────────────┘
    │
    ▼
  3.3V LDO ──────────────────────► VCC
```

电压分压比: Vadc = Vbat × R2/(R1+R2) = Vbat × 0.5

## 3. 电池电量检测

### 3.1 电压-电量映射

锂电池放电曲线（3.7V 标称）：

| 电压 (V) | 电量 (%) | 状态 |
|----------|----------|------|
| 4.20 | 100% | 充满 |
| 4.00 | 80% | |
| 3.80 | 60% | |
| 3.70 | 40% | 正常 |
| 3.50 | 20% | |
| 3.30 | 5% | 低电量 |
| 3.00 | 0% | 关机 |

### 3.2 实现代码

```c
// battery.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

// 初始化电池检测
void battery_init(void);

// 获取电池电压 (mV)
uint16_t battery_get_voltage(void);

// 获取电池电量 (0-100%)
uint8_t battery_get_percent(void);

// 是否正在充电
bool battery_is_charging(void);

// 是否低电量
bool battery_is_low(void);

// 电池状态任务
void battery_task(void);
```

```c
// battery.c

#include "battery.h"
#include "CH58x_common.h"

#define BATTERY_SAMPLE_INTERVAL_MS  5000  // 5秒采样一次
#define BATTERY_LOW_THRESHOLD       10    // 10% 低电量阈值
#define BATTERY_CRITICAL_THRESHOLD  5     // 5% 极低电量

// ADC 参考电压 (mV)
#define ADC_VREF_MV     3300
// ADC 分辨率
#define ADC_RESOLUTION  4096
// 分压比 (0.5)
#define VOLTAGE_DIVIDER 2

// 电压-电量查找表 (电压单位: mV)
static const struct {
    uint16_t voltage;
    uint8_t percent;
} voltage_table[] = {
    {4200, 100},
    {4100, 90},
    {4000, 80},
    {3900, 70},
    {3800, 60},
    {3700, 50},
    {3600, 40},
    {3500, 30},
    {3400, 20},
    {3300, 10},
    {3200, 5},
    {3000, 0},
};

static struct {
    uint16_t voltage_mv;
    uint8_t percent;
    bool charging;
    uint32_t last_sample_time;
} battery_state;

void battery_init(void) {
    // 配置 ADC
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_0);

    // 配置充电检测引脚
    GPIOB_ModeCfg(GPIO_Pin_12, GPIO_ModeIN_PU);

    // 首次采样
    battery_sample();
}

static void battery_sample(void) {
    // 选择通道 (PA4 = CH4)
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_0);

    // 启动采样
    R8_ADC_CHANNEL = 4;  // PA4
    R8_ADC_CFG |= RB_ADC_POWER_ON;

    // 等待转换完成
    while (!(R8_ADC_CONVERT & RB_ADC_EOC_FLAG))
        ;

    // 读取结果
    uint16_t adc_value = R16_ADC_DATA;

    // 清除标志
    R8_ADC_CONVERT &= ~RB_ADC_EOC_FLAG;

    // 计算电压 (考虑分压)
    // Vbat = (ADC_value / 4096) * 3300mV * 2
    battery_state.voltage_mv = (uint32_t)adc_value * ADC_VREF_MV * VOLTAGE_DIVIDER / ADC_RESOLUTION;

    // 查表计算电量
    battery_state.percent = voltage_to_percent(battery_state.voltage_mv);

    // 更新充电状态
    battery_state.charging = (GPIOB_ReadPortPin(GPIO_Pin_12) == 0);

    battery_state.last_sample_time = timer_read32();
}

static uint8_t voltage_to_percent(uint16_t voltage_mv) {
    // 线性插值查表
    uint8_t table_size = sizeof(voltage_table) / sizeof(voltage_table[0]);

    if (voltage_mv >= voltage_table[0].voltage) {
        return 100;
    }
    if (voltage_mv <= voltage_table[table_size - 1].voltage) {
        return 0;
    }

    for (uint8_t i = 0; i < table_size - 1; i++) {
        if (voltage_mv <= voltage_table[i].voltage &&
            voltage_mv > voltage_table[i + 1].voltage) {
            // 线性插值
            uint16_t v_high = voltage_table[i].voltage;
            uint16_t v_low = voltage_table[i + 1].voltage;
            uint8_t p_high = voltage_table[i].percent;
            uint8_t p_low = voltage_table[i + 1].percent;

            return p_low + (voltage_mv - v_low) * (p_high - p_low) / (v_high - v_low);
        }
    }

    return 0;
}

uint16_t battery_get_voltage(void) {
    return battery_state.voltage_mv;
}

uint8_t battery_get_percent(void) {
    return battery_state.percent;
}

bool battery_is_charging(void) {
    return battery_state.charging;
}

bool battery_is_low(void) {
    return battery_state.percent <= BATTERY_LOW_THRESHOLD;
}

void battery_task(void) {
    // 定期采样
    if (timer_elapsed32(battery_state.last_sample_time) >= BATTERY_SAMPLE_INTERVAL_MS) {
        battery_sample();

        // 低电量警告
        if (battery_state.percent <= BATTERY_CRITICAL_THRESHOLD) {
            // 触发低电量动作
            on_critical_battery();
        } else if (battery_state.percent <= BATTERY_LOW_THRESHOLD) {
            on_low_battery();
        }
    }
}

static void on_low_battery(void) {
    // 低电量时的处理
    // 例如: RGB 降低亮度，显示低电量指示
    #ifdef RGB_MATRIX_ENABLE
    rgb_matrix_set_speed(rgb_matrix_get_speed() / 2);
    #endif
}

static void on_critical_battery(void) {
    // 极低电量时的处理
    // 例如: 关闭 RGB，准备关机
    #ifdef RGB_MATRIX_ENABLE
    rgb_matrix_disable_noeeprom();
    #endif

    // 保存状态
    // eeconfig_update_...

    // 可选: 自动关机
    // enter_deep_sleep();
}
```

## 4. 低功耗策略

### 4.1 功耗模式定义

| 模式 | 触发条件 | 矩阵扫描 | RGB | BLE | 预计功耗 |
|------|----------|----------|-----|-----|----------|
| Active | 按键中 | 1kHz | 开启 | 活动 | ~30mA |
| Normal | 按键后5秒 | 100Hz | 降低 | 活动 | ~10mA |
| Idle | 30秒无操作 | 10Hz | 关闭 | Slave Latency | ~2mA |
| Sleep | 5分钟无操作 | 中断 | 关闭 | 断开 | ~50uA |
| Deep Sleep | 30分钟无操作 | 中断 | 关闭 | 关闭 | ~5uA |

### 4.2 实现代码

```c
// power_mode.h
#pragma once

typedef enum {
    POWER_MODE_ACTIVE,
    POWER_MODE_NORMAL,
    POWER_MODE_IDLE,
    POWER_MODE_SLEEP,
    POWER_MODE_DEEP_SLEEP,
} power_mode_t;

void power_mode_init(void);
void power_mode_task(void);
void power_mode_on_activity(void);
power_mode_t power_mode_get(void);
```

```c
// power_mode.c

#include "power_mode.h"

#define NORMAL_TIMEOUT_MS       5000      // 5秒
#define IDLE_TIMEOUT_MS         30000     // 30秒
#define SLEEP_TIMEOUT_MS        300000    // 5分钟
#define DEEP_SLEEP_TIMEOUT_MS   1800000   // 30分钟

static power_mode_t current_mode = POWER_MODE_ACTIVE;
static uint32_t last_activity_time = 0;

void power_mode_init(void) {
    last_activity_time = timer_read32();
}

void power_mode_on_activity(void) {
    last_activity_time = timer_read32();

    if (current_mode != POWER_MODE_ACTIVE) {
        exit_low_power_mode();
        current_mode = POWER_MODE_ACTIVE;
    }
}

void power_mode_task(void) {
    uint32_t elapsed = timer_elapsed32(last_activity_time);
    power_mode_t new_mode = POWER_MODE_ACTIVE;

    if (elapsed >= DEEP_SLEEP_TIMEOUT_MS) {
        new_mode = POWER_MODE_DEEP_SLEEP;
    } else if (elapsed >= SLEEP_TIMEOUT_MS) {
        new_mode = POWER_MODE_SLEEP;
    } else if (elapsed >= IDLE_TIMEOUT_MS) {
        new_mode = POWER_MODE_IDLE;
    } else if (elapsed >= NORMAL_TIMEOUT_MS) {
        new_mode = POWER_MODE_NORMAL;
    }

    if (new_mode != current_mode) {
        apply_power_mode(new_mode);
        current_mode = new_mode;
    }
}

static void apply_power_mode(power_mode_t mode) {
    switch (mode) {
        case POWER_MODE_ACTIVE:
            // 全速模式
            set_matrix_scan_rate(1000);
            #ifdef RGB_MATRIX_ENABLE
            rgb_matrix_enable_noeeprom();
            #endif
            break;

        case POWER_MODE_NORMAL:
            // 正常模式
            set_matrix_scan_rate(100);
            #ifdef RGB_MATRIX_ENABLE
            rgb_matrix_set_speed(64);
            #endif
            break;

        case POWER_MODE_IDLE:
            // 空闲模式
            set_matrix_scan_rate(10);
            #ifdef RGB_MATRIX_ENABLE
            rgb_matrix_disable_noeeprom();
            #endif
            // BLE 增加 slave latency
            #ifdef BLE_ENABLE
            ble_set_conn_params(50, 100, 30);  // 增加延迟
            #endif
            break;

        case POWER_MODE_SLEEP:
            // 睡眠模式
            #ifdef BLE_ENABLE
            ble_disconnect();
            #endif
            enter_sleep_mode();
            break;

        case POWER_MODE_DEEP_SLEEP:
            // 深度睡眠
            enter_deep_sleep_mode();
            break;
    }
}

static void enter_sleep_mode(void) {
    // 配置唤醒源 (矩阵按键)
    configure_wakeup_pins();

    // 停止不必要的外设
    stop_peripherals();

    // 进入低功耗模式
    LowPower_Sleep(RB_PWR_RAM2K | RB_PWR_RAM16K);
}

static void enter_deep_sleep_mode(void) {
    // 保存状态到 EEPROM
    save_state();

    // 配置唤醒源
    configure_wakeup_pins();

    // 进入最低功耗模式
    LowPower_Shutdown(0);
}

static void exit_low_power_mode(void) {
    // 恢复外设
    resume_peripherals();

    // 恢复 BLE 广播
    #ifdef BLE_ENABLE
    ble_start_advertising();
    #endif
}
```

## 5. 充电管理

### 5.1 充电状态检测

```c
bool is_usb_connected(void) {
    // USB VBUS 检测
    return (R8_USB_CTRL & RB_UC_DEV_ATTACH) != 0;
}

bool is_charging(void) {
    // 充电 IC 状态检测
    return battery_is_charging();
}
```

### 5.2 充电时的行为

```c
void charging_behavior(void) {
    if (is_charging()) {
        // 充电时保持 RGB 关闭或低亮度
        #ifdef RGB_MATRIX_ENABLE
        rgb_matrix_set_brightness(32);  // 10% 亮度
        #endif

        // 禁止进入深度睡眠
        disable_deep_sleep();
    }
}
```

## 6. 电量指示

### 6.1 RGB 指示

利用 4 颗状态灯显示电量：

| 电量 | 灯1 | 灯2 | 灯3 | 灯4 |
|------|-----|-----|-----|-----|
| 76-100% | 绿 | 绿 | 绿 | 绿 |
| 51-75% | 绿 | 绿 | 绿 | 灭 |
| 26-50% | 黄 | 黄 | 灭 | 灭 |
| 11-25% | 橙 | 灭 | 灭 | 灭 |
| 0-10% | 红(闪) | 灭 | 灭 | 灭 |
| 充电中 | 蓝(呼吸) | - | - | - |

```c
void battery_indicator_update(void) {
    uint8_t percent = battery_get_percent();
    bool charging = battery_is_charging();

    if (charging) {
        // 充电呼吸灯
        set_indicator_breathing(LED_BLUE);
    } else if (percent > 75) {
        set_indicator(4, LED_GREEN);
    } else if (percent > 50) {
        set_indicator(3, LED_GREEN);
    } else if (percent > 25) {
        set_indicator(2, LED_YELLOW);
    } else if (percent > 10) {
        set_indicator(1, LED_ORANGE);
    } else {
        // 低电量闪烁
        set_indicator_blink(1, LED_RED);
    }
}
```

---

*文档创建日期: 2026-01-19*
