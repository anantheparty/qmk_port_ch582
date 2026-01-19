# 2.4G 接收器固件设计

## 1. 概述

本文档描述 Obey65 键盘配套 2.4G 接收器的固件设计。接收器负责接收键盘的无线数据并转换为 USB HID 报告。

## 2. 硬件选型

### 2.1 方案对比

| 方案 | 芯片 | 优点 | 缺点 |
|------|------|------|------|
| 方案A | CH582 | 与键盘芯片相同，代码复用 | 成本较高，体积大 |
| 方案B | CH552 | 成本低，体积小 | 无2.4G，需要外挂RF |
| 方案C | nRF24 + MCU | 成熟方案 | 需要两个芯片 |

**推荐**: 方案A (CH582)，开发效率高，可复用代码。

### 2.2 硬件设计

```
┌─────────────────────────────────────────┐
│           CH582 接收器                   │
│  ┌─────────┐  ┌─────────┐              │
│  │  USB    │  │  2.4G   │              │
│  │ Device  │  │   RF    │              │
│  └────┬────┘  └────┬────┘              │
│       │            │                    │
│       │            │                    │
│  ┌────┴────────────┴────┐              │
│  │      CH582M          │              │
│  │  (RISC-V + BLE/RF)   │              │
│  └──────────────────────┘              │
│                                         │
│  [LED]  [Button]                        │
└─────────────────────────────────────────┘
```

引脚分配：
- USB: 内置
- LED: PA8 (状态指示)
- Button: PA9 (配对按钮)

## 3. 软件架构

### 3.1 模块结构

```
┌─────────────────────────────────────────────────────────────────┐
│                           main.c                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    主循环                                 │   │
│  │  - 初始化                                                 │   │
│  │  - TMOS 调度                                              │   │
│  │  - 状态管理                                               │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
        │                    │                    │
        ▼                    ▼                    ▼
┌───────────────┐  ┌───────────────┐  ┌───────────────┐
│   esb_rx.c    │  │   usb_hid.c   │  │   led.c       │
│  ┌─────────┐  │  │  ┌─────────┐  │  │  ┌─────────┐  │
│  │RF 接收  │  │  │  │USB HID  │  │  │  │状态指示 │  │
│  │数据解析 │  │  │  │报告发送 │  │  │  │         │  │
│  │配对处理 │  │  │  │描述符   │  │  │  │         │  │
│  └─────────┘  │  │  └─────────┘  │  │  └─────────┘  │
└───────────────┘  └───────────────┘  └───────────────┘
```

### 3.2 状态机

```
                    ┌─────────────────┐
                    │   POWER_ON      │
                    │   (上电)        │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │   INIT          │
                    │   (初始化)      │
                    └────────┬────────┘
                             │
                      ┌──────┴──────┐
                      │             │
                   已配对        未配对
                      │             │
                      ▼             ▼
            ┌─────────────┐  ┌─────────────┐
            │  SCANNING   │  │  PAIRING    │
            │  (扫描中)   │  │  (配对模式) │
            └──────┬──────┘  └──────┬──────┘
                   │                │
                   │ 收到数据       │ 配对成功
                   ▼                │
            ┌─────────────┐         │
            │  CONNECTED  │◄────────┘
            │  (已连接)   │
            └─────────────┘
```

## 4. 核心模块设计

### 4.1 RF 接收模块 (esb_rx.c)

```c
// esb_rx.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

// 初始化 RF 接收
void esb_rx_init(void);

// 进入配对模式
void esb_rx_enter_pairing(void);

// 退出配对模式
void esb_rx_exit_pairing(void);

// RF 接收任务 (在主循环调用)
void esb_rx_task(void);

// 检查是否已配对
bool esb_rx_is_paired(void);

// 检查是否连接
bool esb_rx_is_connected(void);

// 获取最新的键盘报告
bool esb_rx_get_keyboard_report(uint8_t *report, uint8_t *len);

// 获取最新的鼠标报告
bool esb_rx_get_mouse_report(uint8_t *report, uint8_t *len);
```

```c
// esb_rx.c

#include "esb_rx.h"
#include "CH58xBLE_LIB.H"

static struct {
    bool paired;
    bool connected;
    uint8_t keyboard_addr[4];
    uint8_t last_seq;
    uint32_t last_rx_time;

    // 接收缓冲
    uint8_t kb_report[8];
    bool kb_report_pending;
} esb_state;

void esb_rx_init(void) {
    // 加载配对信息
    load_pairing_info();

    // 初始化 RF
    // 使用 BLE 库的 RF 功能或直接操作寄存器
    RF_Init();

    if (esb_state.paired) {
        // 设置为键盘地址
        RF_SetAddress(esb_state.keyboard_addr);
        // 开始接收
        RF_StartRx();
    }
}

void esb_rx_task(void) {
    // 检查是否收到数据
    if (RF_DataReady()) {
        uint8_t buffer[32];
        uint8_t len = RF_ReadData(buffer);

        process_received_data(buffer, len);
    }

    // 连接超时检测
    if (esb_state.connected) {
        if (timer_elapsed32(esb_state.last_rx_time) > 1000) {
            // 1秒无数据，认为断开
            esb_state.connected = false;
        }
    }
}

static void process_received_data(uint8_t *data, uint8_t len) {
    uint8_t msg_type = data[0];
    uint8_t seq = data[1];

    // 检查序列号，过滤重复
    if (seq == esb_state.last_seq) {
        return;
    }
    esb_state.last_seq = seq;
    esb_state.last_rx_time = timer_read32();
    esb_state.connected = true;

    switch (msg_type) {
        case ESB_MSG_KEYBOARD:
            memcpy(esb_state.kb_report, &data[2], 8);
            esb_state.kb_report_pending = true;
            // 发送 ACK
            send_ack(seq);
            break;

        case ESB_MSG_PAIRING_REQ:
            if (is_pairing_mode) {
                handle_pairing_request(data, len);
            }
            break;

        case ESB_MSG_PING:
            send_pong();
            break;
    }
}

bool esb_rx_get_keyboard_report(uint8_t *report, uint8_t *len) {
    if (esb_state.kb_report_pending) {
        memcpy(report, esb_state.kb_report, 8);
        *len = 8;
        esb_state.kb_report_pending = false;
        return true;
    }
    return false;
}
```

### 4.2 USB HID 模块 (usb_hid.c)

```c
// usb_hid.h
#pragma once

#include <stdint.h>

// 初始化 USB HID
void usb_hid_init(void);

// USB 任务
void usb_hid_task(void);

// 发送键盘报告
void usb_hid_send_keyboard(uint8_t *report, uint8_t len);

// 发送鼠标报告
void usb_hid_send_mouse(uint8_t *report, uint8_t len);

// 检查 USB 是否就绪
bool usb_hid_ready(void);
```

```c
// usb_hid.c

#include "usb_hid.h"
#include "usbd_core.h"
#include "usbd_hid.h"

// HID 报告描述符
static const uint8_t hid_report_desc[] = {
    // 键盘
    0x05, 0x01,  // Usage Page (Generic Desktop)
    0x09, 0x06,  // Usage (Keyboard)
    0xA1, 0x01,  // Collection (Application)
    // ... (与键盘端相同)
    0xC0,

    // 鼠标
    0x05, 0x01,  // Usage Page (Generic Desktop)
    0x09, 0x02,  // Usage (Mouse)
    0xA1, 0x01,  // Collection (Application)
    // ...
    0xC0,
};

void usb_hid_init(void) {
    usbd_desc_register(hid_descriptor);
    usbd_add_interface(usbd_hid_init_intf(&hid_class, hid_report_desc, sizeof(hid_report_desc)));
    usbd_initialize();
}

void usb_hid_task(void) {
    // USB 轮询
    usbd_poll();
}

void usb_hid_send_keyboard(uint8_t *report, uint8_t len) {
    usbd_ep_start_write(HID_EP_IN, report, len);
}
```

### 4.3 LED 状态指示 (led.c)

```c
// LED 状态定义
typedef enum {
    LED_OFF,
    LED_ON,
    LED_BLINK_SLOW,    // 1Hz - 未配对
    LED_BLINK_FAST,    // 5Hz - 配对中
    LED_BLINK_ONCE,    // 闪一下 - 收到数据
} led_mode_t;

static led_mode_t current_led_mode = LED_OFF;

void led_set_mode(led_mode_t mode) {
    current_led_mode = mode;
}

void led_task(void) {
    static uint32_t last_toggle = 0;
    uint32_t interval = 0;

    switch (current_led_mode) {
        case LED_OFF:
            gpio_write_pin_low(LED_PIN);
            break;
        case LED_ON:
            gpio_write_pin_high(LED_PIN);
            break;
        case LED_BLINK_SLOW:
            interval = 500;
            break;
        case LED_BLINK_FAST:
            interval = 100;
            break;
    }

    if (interval > 0 && timer_elapsed32(last_toggle) >= interval) {
        gpio_toggle_pin(LED_PIN);
        last_toggle = timer_read32();
    }
}
```

## 5. 主程序

```c
// main.c

#include "CH58x_common.h"
#include "esb_rx.h"
#include "usb_hid.h"
#include "led.h"

int main(void) {
    // 系统初始化
    SystemCoreClockUpdate();

    // 模块初始化
    led_init();
    usb_hid_init();
    esb_rx_init();

    // 检查是否已配对
    if (!esb_rx_is_paired()) {
        led_set_mode(LED_BLINK_SLOW);
    }

    // 检查配对按钮
    if (gpio_read_pin(BUTTON_PIN) == 0) {
        esb_rx_enter_pairing();
        led_set_mode(LED_BLINK_FAST);
    }

    // 主循环
    while (1) {
        // USB 任务
        usb_hid_task();

        // RF 接收任务
        esb_rx_task();

        // LED 任务
        led_task();

        // 处理接收到的报告
        uint8_t report[8];
        uint8_t len;

        if (usb_hid_ready() && esb_rx_get_keyboard_report(report, &len)) {
            usb_hid_send_keyboard(report, len);
            led_set_mode(LED_BLINK_ONCE);
        }

        // 更新连接状态 LED
        if (esb_rx_is_connected()) {
            led_set_mode(LED_ON);
        } else if (esb_rx_is_paired()) {
            led_set_mode(LED_BLINK_SLOW);
        }
    }

    return 0;
}
```

## 6. 构建配置

### 6.1 CMakeLists.txt

```cmake
# 接收器固件单独的构建目标
set(DONGLE_TARGET ${keyboard}_dongle)

add_executable(${DONGLE_TARGET}.elf
    dongle/main.c
    dongle/esb_rx.c
    dongle/usb_hid.c
    dongle/led.c
)

target_compile_definitions(${DONGLE_TARGET}.elf PUBLIC
    __BUILDING_DONGLE__=1
)

target_link_libraries(${DONGLE_TARGET}.elf PUBLIC
    CH582_APP
)
```

### 6.2 构建命令

```bash
# 构建接收器固件
cmake -Dkeyboard=obey65 -DESB_ROLE=dongle ..
make
```

## 7. 测试方案

### 7.1 基础功能测试

- [ ] USB 枚举正常
- [ ] LED 指示正确
- [ ] 配对按钮响应

### 7.2 通信测试

- [ ] 能接收键盘数据
- [ ] USB HID 报告正确
- [ ] 延迟在可接受范围

### 7.3 稳定性测试

- [ ] 长时间运行无崩溃
- [ ] 断开重连正常
- [ ] 干扰环境下稳定

---

*文档创建日期: 2026-01-19*
