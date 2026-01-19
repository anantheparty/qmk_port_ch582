---
name: ch582-hardware
description: CH582 RISC-V BLE SoC 硬件开发指南。当任务涉及 CH582 芯片、GPIO、外设配置、BLE、低功耗、寄存器操作时使用。
---

# CH582 硬件开发指南

## 芯片概述
- **架构**: RISC-V4A (青稀 V4A)
- **主频**: 最高 80MHz (PLL)
- **Flash**: 448KB (部分型号 512KB)
- **RAM**: 32KB
- **BLE**: 5.3，支持多连接

## GPIO 配置

### 引脚复用
```c
// 设置为推挽输出
GPIOB_SetBits(GPIO_Pin_4);
GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeOut_PP_5mA);

// 设置为输入上拉
GPIOB_ModeCfg(GPIO_Pin_5, GPIO_ModeIN_PU);

// 复用功能 (如 UART)
GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeIN_PU);  // RX
GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeOut_PP_5mA);  // TX
```

### 键盘矩阵常用引脚
| 功能 | 推荐引脚 | 备注 |
|------|----------|------|
| 行 (Row) | PA4-PA15, PB4-PB23 | 输出 |
| 列 (Col) | 同上 | 输入上拉 |
| WS2812 | PB0 (SPI MOSI) 或 PA12 (PWM4) | |
| I2C | PB14 (SDA), PB13 (SCL) | |

## 外设配置

### SPI (用于 WS2812/AW20216S)
```c
// SPI0 配置
SPI0_MasterDefInit();
SPI0_CLKCfg(4);  // 分频
SPI0_DataMode(Mode_DataStream);
```

### PWM (用于 RGB/Backlight)
```c
PWMX_CLKCfg(4);  // 系统时钟 4 分频
PWMX_CycleCfg(PWMX_Cycle_64);
TMR1_PWMInit(High_Level, PWM_Times_1);
```

### 低功耗模式
```c
// 进入睡眠
LowPower_Sleep(RB_PWR_RAM30K | RB_PWR_RAM2K);

// 唤醒源配置
GPIOB_ITModeCfg(GPIO_Pin_X, GPIO_ITMode_FallEdge);
PFIC_EnableIRQ(GPIO_B_IRQn);
```

## BLE 相关

### 广播配置
```c
// 设置广播间隔 (单位: 0.625ms)
uint16_t advInt = 160;  // 100ms
GAPRole_SetParameter(GAPROLE_ADV_INTERVAL, sizeof(uint16_t), &advInt);
```

### HID 服务
- 使用 WCH BLE HID 例程作为基础
- Report Descriptor 需符合 USB HID 规范

## 注意事项

### Flash 写入
- Flash 操作需在 RAM 中执行或禁用中断
- 最小擦除单位: 256 字节

### 时钟配置
```c
// 使用外部 32MHz 晶振
SetSysClock(CLK_SOURCE_PLL_80MHz);
```

### 中断优先级
- CH582 使用 PFIC (可编程快速中断控制器)
- 注意 QMK 定时器中断优先级设置
