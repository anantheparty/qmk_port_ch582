/**
 * BLE HID 协议实现
 *
 * 实现 Obey65 键盘的蓝牙 HID 功能：
 * - GAP Peripheral 角色配置
 * - HID over GATT 服务
 * - 键盘/鼠标报告发送
 * - 连接状态管理
 */

#include "protocol_ble.h"
#include "CH58xBLE_LIB.H"
#include "config.h"
#include "hid_dev.h"
#include "ble_compat.h"
#include "report.h"

#ifdef DEBUG_UART_ENABLE
#include "debug_uart.h"
#endif

// ============================================================================
// BLE 状态管理
// ============================================================================

// Task ID for BLE
static uint8_t bleTaskId = INVALID_TASK_ID;

// 当前连接句柄 (0xFFFF 表示未连接)
static uint16_t bleConnHandle = GAP_CONNHANDLE_INIT;

// BLE 连接状态
typedef enum {
    BLE_STATE_IDLE = 0,      // 空闲
    BLE_STATE_ADVERTISING,   // 广播中
    BLE_STATE_CONNECTED,     // 已连接
    BLE_STATE_BONDED         // 已绑定
} ble_state_t;

static ble_state_t bleState = BLE_STATE_IDLE;

// ============================================================================
// 广播和扫描响应数据
// ============================================================================

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8_t scanRspData[] = {
    // complete name
    0x07,   // length of this data
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'O', 'b', 'e', 'y', '6', '5',

    // connection interval range
    0x05,   // length of this data
    GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
    LO_UINT16(6),    // min: 7.5ms (6 * 1.25ms)
    HI_UINT16(6),
    LO_UINT16(12),   // max: 15ms (12 * 1.25ms) - 低延迟键盘
    HI_UINT16(12),

    // Tx power level
    0x02,   // length of this data
    GAP_ADTYPE_POWER_LEVEL,
    0       // 0dBm
};

// GAP - Advertisement data (max size = 31 bytes)
static uint8_t advertData[] = {
    // Flags - 使用 GENERAL 模式以便无限期广播
    0x02,   // length of this data
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    // Appearance - Keyboard
    0x03,   // length of this data
    GAP_ADTYPE_APPEARANCE,
    LO_UINT16(GAP_APPEARE_HID_KEYBOARD),
    HI_UINT16(GAP_APPEARE_HID_KEYBOARD),

    // Service UUIDs - HID
    0x03,   // length of this data
    GAP_ADTYPE_16BIT_MORE,
    LO_UINT16(HID_SERV_UUID),
    HI_UINT16(HID_SERV_UUID),
};

// ============================================================================
// GAP 回调函数
// ============================================================================

/**
 * GAP Role 状态变化回调
 */
static void ble_StateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent) {
    switch (newState & GAPROLE_STATE_ADV_MASK) {
        case GAPROLE_STARTED:
#ifdef DEBUG_UART_ENABLE
            DEBUG_PRINT("BLE: Started\n");
#endif
            bleState = BLE_STATE_IDLE;
            break;

        case GAPROLE_ADVERTISING:
#ifdef DEBUG_UART_ENABLE
            DEBUG_PRINT("BLE: Advertising\n");
#endif
            bleState = BLE_STATE_ADVERTISING;
            break;

        case GAPROLE_CONNECTED:
            if (pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT) {
                bleConnHandle = pEvent->linkCmpl.connectionHandle;
                HidDev_SetConnHandle(bleConnHandle);
#ifdef DEBUG_UART_ENABLE
                DEBUG_PRINT("BLE: Connected, handle=%d\n", bleConnHandle);
#endif
                bleState = BLE_STATE_CONNECTED;
            }
            break;

        case GAPROLE_CONNECTED_ADV:
#ifdef DEBUG_UART_ENABLE
            DEBUG_PRINT("BLE: Connected + Advertising\n");
#endif
            break;

        case GAPROLE_WAITING:
            if (pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT) {
#ifdef DEBUG_UART_ENABLE
                DEBUG_PRINT("BLE: Disconnected, reason=%d\n", pEvent->linkTerminate.reason);
#endif
                bleConnHandle = GAP_CONNHANDLE_INIT;
                HidDev_SetConnHandle(GAP_CONNHANDLE_INIT);
                bleState = BLE_STATE_IDLE;

                // 断开后重新开始广播
                uint8_t adv_enable = TRUE;
                GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &adv_enable);
            }
            break;

        case GAPROLE_ERROR:
#ifdef DEBUG_UART_ENABLE
            DEBUG_PRINT("BLE: Error\n");
#endif
            break;

        default:
            break;
    }
}

// GAP Role Callbacks
static gapRolesCBs_t ble_PeripheralCBs = {
    ble_StateNotificationCB,  // Profile State Change Callbacks
    NULL,                     // RSSI callback
    NULL                      // Parameter update callback
};

/**
 * 配对状态回调
 */
static void ble_PairStateCB(uint16_t connHandle, uint8_t state, uint8_t status) {
    if (state == GAPBOND_PAIRING_STATE_COMPLETE) {
        if (status == SUCCESS) {
#ifdef DEBUG_UART_ENABLE
            DEBUG_PRINT("BLE: Pairing complete\n");
#endif
            bleState = BLE_STATE_BONDED;
        } else {
#ifdef DEBUG_UART_ENABLE
            DEBUG_PRINT("BLE: Pairing failed, status=%d\n", status);
#endif
        }
    } else if (state == GAPBOND_PAIRING_STATE_BONDED) {
#ifdef DEBUG_UART_ENABLE
        DEBUG_PRINT("BLE: Bonded\n");
#endif
        bleState = BLE_STATE_BONDED;
    }
}

/**
 * 密码回调
 */
static void ble_PasscodeCB(uint8_t *deviceAddr, uint16_t connHandle, uint8_t uiInputs, uint8_t uiOutputs) {
    // 自动接受配对（使用默认密码 000000）
    uint32_t passcode = 0;
    GAPBondMgr_PasscodeRsp(connHandle, SUCCESS, passcode);
#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINT("BLE: Passcode request, auto-accept\n");
#endif
}

// Bond Manager Callbacks
static gapBondCBs_t ble_BondMgrCBs = {
    ble_PasscodeCB,    // Passcode callback
    ble_PairStateCB    // Pairing / Bonding state Callback
};

// ============================================================================
// BLE 任务处理
// ============================================================================

static uint16_t BLE_Task_ProcessEvent(uint8_t task_id, uint16_t events);

// ============================================================================
// 协议接口实现
// ============================================================================

static void platform_initialize(void) {
#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINT("BLE: Initializing...\n");
#endif

    // Register TMOS task
    bleTaskId = TMOS_ProcessEventRegister(BLE_Task_ProcessEvent);

    // Setup GAP Peripheral Role Profile
    {
        uint8_t  initial_advertising_enable = TRUE;
        uint16_t desired_min_interval = 6;   // 7.5ms (低延迟)
        uint16_t desired_max_interval = 12;  // 15ms

        // Set the GAP Role Parameters
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
        GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);
        GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16_t), &desired_min_interval);
        GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16_t), &desired_max_interval);
    }

    // Set the GAP Characteristics
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, sizeof("Obey65"), "Obey65");

    // Setup the GAP Bond Manager
    {
        uint32_t passkey = 0; // passkey "000000"
        uint8_t  pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
        uint8_t  mitm = FALSE;  // 不需要 MITM 保护（简化配对）
        uint8_t  ioCap = GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT;
        uint8_t  bonding = TRUE;

        GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE, sizeof(uint32_t), &passkey);
        GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(uint8_t), &pairMode);
        GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION, sizeof(uint8_t), &mitm);
        GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
        GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED, sizeof(uint8_t), &bonding);
    }

    // Initialize GATT attributes
    GGS_AddService(GATT_ALL_SERVICES);         // GAP Service
    GATTServApp_AddService(GATT_ALL_SERVICES); // GATT attributes
    HidDev_AddService();                       // HID Service

    // Start the Device
    GAPRole_PeripheralStartDevice(bleTaskId, &ble_BondMgrCBs, &ble_PeripheralCBs);

#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINT("BLE: Initialized, taskId=%d\n", bleTaskId);
#endif
}

static uint16_t BLE_Task_ProcessEvent(uint8_t task_id, uint16_t events) {
    if (events & SYS_EVENT_MSG) {
        uint8_t *pMsg;

        if ((pMsg = tmos_msg_receive(bleTaskId)) != NULL) {
            // 处理 BLE 消息
            // TODO: 根据消息类型进行处理

            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }

        return (events ^ SYS_EVENT_MSG);
    }

    return 0;
}

static void protocol_setup(void) {
    // BLE protocol setup
}

static void protocol_init(void) {
    // Init BLE hardware
    GAPRole_PeripheralInit();
}

static void protocol_pre_task(void) {
    // BLE pre-task
}

static void protocol_post_task(void) {
    // BLE post-task
}

static void platform_run(void) {
    // BLE main loop step
    TMOS_SystemProcess();
}

static void platform_reboot(void) {
    SYS_ResetExecute();
}

// ============================================================================
// HID 报告发送
// ============================================================================

static void send_keyboard(report_keyboard_t *report) {
    if (bleState < BLE_STATE_CONNECTED || bleConnHandle == GAP_CONNHANDLE_INIT) {
        return;  // 未连接时不发送
    }

    HidDev_Report(HID_RPT_ID_KEYBOARD_IN, HID_REPORT_TYPE_INPUT, 8, (uint8_t *)report);
}

static void send_nkro(report_nkro_t *report) {
    // TODO: 实现 NKRO 报告发送
    // BLE HID 通常不支持完整的 NKRO，可能需要拆分为多个报告
}

static void send_mouse(report_mouse_t *report) {
#ifdef MOUSE_ENABLE
    if (bleState < BLE_STATE_CONNECTED || bleConnHandle == GAP_CONNHANDLE_INIT) {
        return;
    }

    HidDev_Report(HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, 5, (uint8_t *)report);
#endif
}

static void send_extra(report_extra_t *report) {
    if (bleState < BLE_STATE_CONNECTED || bleConnHandle == GAP_CONNHANDLE_INIT) {
        return;
    }

    if (report->report_id == REPORT_ID_CONSUMER) {
        // Consumer Control report (2 bytes)
        HidDev_Report(HID_RPT_ID_CONSUMER_IN, HID_REPORT_TYPE_INPUT, 2, (uint8_t *)&report->usage);
    } else if (report->report_id == REPORT_ID_SYSTEM) {
        // System Control report (1 byte)
        uint8_t system_data = report->usage & 0xFF;
        HidDev_Report(HID_RPT_ID_SYSTEM_IN, HID_REPORT_TYPE_INPUT, 1, &system_data);
    }
}

static uint8_t ble_keyboard_leds(void) {
    return HidDev_GetKeyboardLeds();
}

// ============================================================================
// 协议接口定义
// ============================================================================

const ch582_interface_t ch582_protocol_ble = {
    .ch582_common_driver.keyboard_leds = ble_keyboard_leds,
    .ch582_common_driver.send_keyboard = send_keyboard,
    .ch582_common_driver.send_nkro = send_nkro,
    .ch582_common_driver.send_mouse = send_mouse,
    .ch582_common_driver.send_extra = send_extra,
    .ch582_platform_initialize = platform_initialize,
    .ch582_protocol_setup = protocol_setup,
    .ch582_protocol_init = protocol_init,
    .ch582_protocol_pre_task = protocol_pre_task,
    .ch582_protocol_post_task = protocol_post_task,
    .ch582_platform_run = platform_run,
    .ch582_platform_reboot = platform_reboot,
};

// ============================================================================
// 公共 API
// ============================================================================

/**
 * 获取当前 BLE 连接状态
 */
bool ble_is_connected(void) {
    return bleState >= BLE_STATE_CONNECTED;
}

/**
 * 开始广播
 */
void ble_start_advertising(void) {
    uint8_t adv_enable = TRUE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &adv_enable);
}

/**
 * 停止广播
 */
void ble_stop_advertising(void) {
    uint8_t adv_enable = FALSE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &adv_enable);
}

/**
 * 断开当前连接
 */
void ble_disconnect(void) {
    if (bleConnHandle != GAP_CONNHANDLE_INIT) {
        GAPRole_TerminateLink(bleConnHandle);
    }
}
