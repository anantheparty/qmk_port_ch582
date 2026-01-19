# Phase 3: 2.4G ESB 开发总结

**完成日期**: 2026-01-19
**开发周期**: 1 天
**提交数量**: 2 个

## 概述

Phase 3 成功实现了 Obey65 键盘的 2.4G ESB (Enhanced ShockBurst) 无线协议，包括 RF 初始化、HID 报告发送、ACK/重传机制、配对协议和功耗优化。

## 固件状态

```
Memory region         Used Size  Region Size  %age Used
       FLASH:      231972 B       372 KB     60.90%
         RAM:       28356 B        32 KB     86.54%
```

ESB 功能与 BLE 共享大部分 RF 代码，额外增加约 360B FLASH。

---

## 主要成果

### Phase 3.0-3.3: ESB 协议基础实现

**commit**: 981a8f70

#### RF 配置

```c
static rfConfig_t esb_rf_config = {
    .LLEMode = 0x01 |      // Auto mode (ACK enabled)
               (0x01 << 4) | // 2Mbps PHY
               (0x01 << 6),  // Use frequency instead of channel
    .Frequency = ESB_BASE_FREQ + esb_freq_table[0] * 1000,
    .accessAddress = ESB_ACCESS_ADDRESS,  // 0xE7E7E7E7
    .CRCInit = ESB_CRC_INIT,             // 0x555555
    .rfStatusCB = esb_rf_callback,
    .RxMaxlen = ESB_MAX_PAYLOAD_LEN,
    .TxMaxlen = ESB_MAX_PAYLOAD_LEN
};
```

#### 频率跳变设计

避开 WiFi 信道 1, 6, 11，使用 16 个跳频点：

```c
static const uint8_t esb_freq_table[ESB_CHANNEL_COUNT] = {
    2, 26, 50, 74,   // 避开 WiFi Ch 1, 6, 11
    5, 29, 53, 77,
    8, 32, 56, 80,
    11, 35, 59, 83
};
```

#### 消息类型定义

| 类型 | 值 | 用途 |
|------|-----|------|
| `ESB_MSG_PAIRING_REQ` | 0x01 | 配对请求 |
| `ESB_MSG_PAIRING_RSP` | 0x02 | 配对响应 |
| `ESB_MSG_PAIRING_ACK` | 0x03 | 配对确认 |
| `ESB_MSG_KEYBOARD` | 0x10 | 键盘报告 |
| `ESB_MSG_MOUSE` | 0x11 | 鼠标报告 |
| `ESB_MSG_CONSUMER` | 0x12 | 媒体键报告 |
| `ESB_MSG_ACK` | 0x80 | 通用 ACK |
| `ESB_MSG_PING` | 0xF0 | 心跳包 |
| `ESB_MSG_LED_STATE` | 0xE0 | LED 状态反馈 |

#### 状态机

```
ESB_STATE_IDLE → ESB_STATE_SCANNING → ESB_STATE_PAIRING
                                           ↓
ESB_STATE_DISCONNECTED ← ESB_STATE_CONNECTED ← ESB_STATE_CONNECTING
```

#### ACK/重传机制

- 最大重传次数: 3 次
- ACK 超时: 10ms
- 重传延迟: 500us

```c
#define ESB_MAX_RETRIES         3
#define ESB_ACK_TIMEOUT_MS      10
#define ESB_RETRY_DELAY_US      500
```

#### 心跳检测

- 心跳间隔: 500ms (低功耗模式: 1000ms)
- 断连阈值: 连续 3 次心跳失败

```c
#define ESB_PING_INTERVAL_MS    500
#define ESB_DISCONNECT_COUNT    3
```

---

### Phase 3.4: ESB 功耗优化

**commit**: 5dd65005

#### 功耗模式集成

```c
// power_mode.c
void power_mode_on_activity(void) {
#ifdef ESB_ENABLE
    if (wireless_mode_get() == WIRELESS_MODE_ESB) {
        esb_exit_low_power();
    }
#endif
}

// 空闲时进入低功耗
case POWER_MODE_IDLE:
#ifdef ESB_ENABLE
    if (wireless_mode_get() == WIRELESS_MODE_ESB) {
        esb_enter_low_power();
    }
#endif
```

#### 模式切换集成

```c
// wireless_mode.c
static void enter_mode(wireless_mode_t mode) {
    case WIRELESS_MODE_ESB:
#ifdef ESB_ENABLE
        if (esb_has_pairing()) {
            esb_reconnect();
        } else {
            esb_start_pairing();
        }
#endif
        break;
}
```

---

## 新增文件

| 文件 | 用途 |
|------|------|
| `wireless/include/esb_types.h` | ESB 协议数据类型定义 |
| `wireless/include/protocol_esb.h` | ESB 协议接口 |
| `wireless/src/protocol_esb.c` | ESB 协议实现 |

---

## 协议设计要点

### 数据包结构

**键盘报告**:
```c
typedef struct {
    uint8_t type;       // ESB_MSG_KEYBOARD
    uint8_t seq;        // 序列号
    uint8_t modifier;   // 修饰键
    uint8_t reserved;
    uint8_t keys[6];    // 按键码
} esb_keyboard_msg_t;   // 10 bytes
```

**配对请求**:
```c
typedef struct {
    uint8_t type;           // ESB_MSG_PAIRING_REQ
    uint8_t kb_addr[4];     // 键盘地址
    uint16_t product_id;    // 产品 ID
    uint8_t firmware_ver;   // 固件版本
} esb_pairing_req_t;        // 8 bytes
```

### 地址生成

使用芯片唯一 ID 生成键盘地址：

```c
uint8_t unique_id[8];
GET_UNIQUE_ID(unique_id);
esb_state.config.kb_addr[0] = 0xE7;
esb_state.config.kb_addr[1] = unique_id[0];
esb_state.config.kb_addr[2] = unique_id[1];
esb_state.config.kb_addr[3] = unique_id[2];
```

---

## 架构概述

```
+-------------------+     +------------------+
|  wireless_mode.c  |---->|  protocol_esb.c  |
+-------------------+     +------------------+
         |                        |
         v                        v
+-------------------+     +------------------+
|   power_mode.c    |     |   esb_types.h    |
+-------------------+     +------------------+
                                  |
                                  v
                          +------------------+
                          |  CH58x RF API    |
                          | (RF_Config, etc) |
                          +------------------+
```

---

## 待完成: 接收器固件

键盘端 ESB 协议已实现，但需要配套的接收器固件：

1. **接收器固件设计**
   - 使用 CH582F 芯片
   - USB HID 设备实现
   - RF 接收和解析

2. **接收器功能**
   - 接收键盘 HID 报告
   - 转发到 USB HID
   - LED 状态反馈
   - 配对流程处理

3. **硬件设计** (已提供原理图)
   - USB 接口
   - 2.4GHz 天线
   - 32MHz 晶振
   - 3.3V LDO

---

## 已知限制

1. **接收器依赖**: 键盘端已实现，但无接收器无法实际测试
2. **单配对**: 当前仅支持与一个接收器配对
3. **信道固定**: 配对后使用固定信道，未实现自适应跳频

---

## 待优化项

1. 接收器固件开发
2. 自适应跳频实现
3. 实际延迟测试
4. 功耗实测验证

---

## 下一步: Phase 4

Phase 4 集成与优化：
- ✅ 三模切换整合 (Phase 4.1)
- ✅ LED 状态指示 (Phase 4.3)
- ⏳ 统一电源管理 (Phase 4.2)
- ⏳ 接收器固件开发

---

*文档生成时间: 2026-01-19*
