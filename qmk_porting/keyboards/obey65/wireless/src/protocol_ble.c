/**
 * BLE Protocol - MINIMAL DIAGNOSTIC VERSION
 *
 * Absolute minimum to test if BLE advertising works at all.
 * No GATT services, no bonding, no QMK task.
 *
 * LED feedback (TMR2 4-LED strip):
 *   All RED       = init complete, waiting for TMOS start
 *   1 GREEN       = GAPROLE_STARTED  (BLE stack initialized)
 *   2 GREEN       = GAPROLE_ADVERTISING (RF is transmitting!)
 *   3 GREEN       = GAPROLE_CONNECTED
 *   All RED blink = GAPROLE_ERROR
 */

#include "protocol_ble.h"
#include "CH58xBLE_LIB.H"
#include "config.h"
#include "hid_dev.h"
#include "ble_compat.h"
#include "report.h"
#include "ws2812_tmr2.h"

#ifdef DEBUG_UART_ENABLE
#include "debug_uart.h"
#endif

// ============================================================================
// LED helpers
// ============================================================================

static const led_obey_t LED_RED   = {.r = 40, .g = 0,  .b = 0};
static const led_obey_t LED_GREEN = {.r = 0,  .g = 40, .b = 0};
static const led_obey_t LED_BLUE  = {.r = 0,  .g = 0,  .b = 40};
static const led_obey_t LED_OFF   = {.r = 0,  .g = 0,  .b = 0};

static void set_leds(uint8_t n_green, bool all_red) {
    // DIAGNOSTIC: WS2812 disabled. Use A11 blink count to signal state:
    //   all_red (error)  → fast blink
    //   n_green=1        → A11 LOW  (off)
    //   n_green=2        → A11 HIGH (on) = advertising
    //   n_green=3        → A11 HIGH (on) = connected
    if (all_red) {
        // blink 3 times fast to signal error
        for (int i = 0; i < 3; i++) {
            GPIOA_SetBits(GPIO_Pin_11);
            DelayMs(100);
            GPIOA_ResetBits(GPIO_Pin_11);
            DelayMs(100);
        }
    } else if (n_green >= 2) {
        GPIOA_SetBits(GPIO_Pin_11);  // on = advertising/connected
    } else {
        GPIOA_ResetBits(GPIO_Pin_11);  // off = idle
    }
}

// ============================================================================
// State
// ============================================================================

static uint8_t bleTaskId   = INVALID_TASK_ID;
static uint16_t bleConnHandle = GAP_CONNHANDLE_INIT;

typedef enum {
    BLE_STATE_IDLE = 0,
    BLE_STATE_ADVERTISING,
    BLE_STATE_CONNECTED,
} ble_state_t;

static ble_state_t bleState = BLE_STATE_IDLE;

// ============================================================================
// Advertising data  (minimal: flags + name)
// ============================================================================

static uint8_t advertData[] = {
    0x02, GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    0x07, GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'O', 'b', 'e', 'y', '6', '5',

    0x03, GAP_ADTYPE_APPEARANCE,
    LO_UINT16(GAP_APPEARE_HID_KEYBOARD),
    HI_UINT16(GAP_APPEARE_HID_KEYBOARD),

    // Distinctive marker: company 0xFFFF + magic bytes 0xCA 0xFE 0xBE 0xEF
    0x06, GAP_ADTYPE_MANUFACTURER_SPECIFIC,
    0xFF, 0xFF,
    0xCA, 0xFE, 0xBE,
};

static uint8_t scanRspData[] = {
    // Local name in scan response too (belt-and-suspenders)
    0x07, GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'O', 'b', 'e', 'y', '6', '5',

    0x05, GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
    LO_UINT16(8), HI_UINT16(8),
    LO_UINT16(8), HI_UINT16(8),
};

// ============================================================================
// GAP callbacks
// ============================================================================

static void ble_StateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent) {
    switch (newState & GAPROLE_STATE_ADV_MASK) {
        case GAPROLE_STARTED: {
            // Do NOT call GAP_ConfigDeviceAddr - use the default public BD_ADDR
            // set by CH58X_BLEInit() via GetMACAddress(). Any call here may break RF.
            bleState = BLE_STATE_IDLE;
            set_leds(1, false);  // 1 green = STARTED
        } break;

        case GAPROLE_ADVERTISING:
            bleState = BLE_STATE_ADVERTISING;
            set_leds(2, false);  // 2 green = ADVERTISING (RF is on!)
            break;

        case GAPROLE_CONNECTED:
            if (pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT) {
                bleConnHandle = pEvent->linkCmpl.connectionHandle;
                HidDev_SetConnHandle(bleConnHandle);
                bleState = BLE_STATE_CONNECTED;
                set_leds(3, false);  // 3 green = CONNECTED
            }
            break;

        case GAPROLE_WAITING:
            // Called on: disconnect, directed-adv timeout, or adv timeout.
            // WCH example re-enables advertising unconditionally here.
            bleConnHandle = GAP_CONNHANDLE_INIT;
            bleState = BLE_STATE_IDLE;
            {
                uint8_t adv = TRUE;
                GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &adv);
            }
            set_leds(1, false);
            break;

        case GAPROLE_ERROR:
            set_leds(0, true);  // all red = ERROR
            break;

        default:
            break;
    }
}

static gapRolesCBs_t ble_PeripheralCBs = {
    ble_StateNotificationCB,
    NULL,
    NULL
};

static void ble_PasscodeCB(uint8_t *deviceAddr, uint16_t connHandle,
                           uint8_t uiInputs, uint8_t uiOutputs) {
    GAPBondMgr_PasscodeRsp(connHandle, SUCCESS, 0);
}

static void ble_PairStateCB(uint16_t connHandle, uint8_t state, uint8_t status) {
}

static gapBondCBs_t ble_BondMgrCBs = {
    ble_PasscodeCB,
    ble_PairStateCB
};

// ============================================================================
// Task events
// ============================================================================

#define BLE_START_DEVICE_EVT  0x0001
#define BLE_RUN_QMK_TASK_EVT  0x0002

static void protocol_pre_task(void);
static void protocol_post_task(void);
extern void protocol_keyboard_task(void);
extern void housekeeping_task(void);

static uint16_t BLE_Task_ProcessEvent(uint8_t task_id, uint16_t events) {
    if (events & SYS_EVENT_MSG) {
        uint8_t *pMsg;
        if ((pMsg = tmos_msg_receive(bleTaskId)) != NULL) {
            tmos_msg_deallocate(pMsg);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    // Deferred start - called from inside TMOS loop (WCH example pattern)
    if (events & BLE_START_DEVICE_EVT) {
        GAPRole_PeripheralStartDevice(bleTaskId, &ble_BondMgrCBs, &ble_PeripheralCBs);
        return (events ^ BLE_START_DEVICE_EVT);
    }

    // DIAGNOSTIC: QMK task DISABLED - test if QMK interferes with BLE RF
    // If keyboard appears in scan, QMK task is the culprit
    if (events & BLE_RUN_QMK_TASK_EVT) {
        // Show actual BLE state on LEDs (no QMK task, no blink)
        switch (bleState) {
            case BLE_STATE_IDLE:        set_leds(1, false); break;
            case BLE_STATE_ADVERTISING: set_leds(2, false); break;
            case BLE_STATE_CONNECTED:   set_leds(3, false); break;
        }
        tmos_start_task(task_id, BLE_RUN_QMK_TASK_EVT, MS1_TO_SYSTEM_TIME(50));
        return (events ^ BLE_RUN_QMK_TASK_EVT);
    }

    return 0;
}

// ============================================================================
// Platform interface
// ============================================================================

static void platform_initialize(void) {
    // DIAGNOSTIC: skip TMR2/WS2812 init - test if TMR2 DMA interferes with BLE RF
    // tmr2_ws2812_init();
    //
    // Instead, use raw GPIO blink on A11 as state indicator (1=init, 2=advertising)
    GPIOA_ModeCfg(GPIO_Pin_11, GPIO_ModeOut_PP_5mA);
    GPIOA_ResetBits(GPIO_Pin_11);  // off during init

    // (No blue LED phase - can't without WS2812)

    // 1. GAPRole init (must be first)
    GAPRole_PeripheralInit();

    // 2. Register TMOS task
    bleTaskId = TMOS_ProcessEventRegister(BLE_Task_ProcessEvent);

    // 3. Set advertising intervals (match WCH official HID_Keyboard example exactly)
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, 48);  // 30ms
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, 80);  // 50ms
    GAP_SetParamValue(TGAP_LIM_ADV_TIMEOUT,  60);  // 60s (safety: also set for general mode)

    // 4. GAP role parameters
    {
        uint8_t  adv_on = TRUE;
        uint16_t conn_min = 8, conn_max = 8;
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED,    sizeof(uint8_t),  &adv_on);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA,       sizeof(advertData), advertData);
        GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA,     sizeof(scanRspData), scanRspData);
        GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16_t), &conn_min);
        GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16_t), &conn_max);
    }

    // 5. Device name
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, "Obey65");

    // 6. Bond manager - DISABLED for minimal diagnostic
    // (GAPBondMgr not needed just to test if advertising works)

    // 7. GATT services - DISABLED for minimal diagnostic
    // If advertising works without GATT, the GATT table is the problem
    GGS_AddService(GATT_ALL_SERVICES);
    GATTServApp_AddService(GATT_ALL_SERVICES);
    // HidDev_AddService();  // <-- DISABLED: test if HID GATT is blocking RF

    // 8. Show all-red = init done, waiting for TMOS to start BLE
    set_leds(0, true);

    // 9. Deferred start (WCH pattern: GAPRole_PeripheralStartDevice from TMOS event)
    tmos_set_event(bleTaskId, BLE_START_DEVICE_EVT);

    // 10. QMK task
    tmos_start_task(bleTaskId, BLE_RUN_QMK_TASK_EVT, MS1_TO_SYSTEM_TIME(10));
}

static void protocol_setup(void) {}

static void protocol_init(void) {}

static void protocol_pre_task(void) {}

static void protocol_post_task(void) {}

static void platform_run(void) {
    TMOS_SystemProcess();
}

static void platform_reboot(void) {
    SYS_ResetExecute();
}

// ============================================================================
// HID report sending
// ============================================================================

static void send_keyboard(report_keyboard_t *report) {
    if (bleState != BLE_STATE_CONNECTED || bleConnHandle == GAP_CONNHANDLE_INIT) return;
    HidDev_Report(HID_RPT_ID_KEYBOARD_IN, HID_REPORT_TYPE_INPUT, 8, (uint8_t *)report);
}

static void send_nkro(report_nkro_t *report) {}

static void send_mouse(report_mouse_t *report) {
#ifdef MOUSE_ENABLE
    if (bleState != BLE_STATE_CONNECTED || bleConnHandle == GAP_CONNHANDLE_INIT) return;
    HidDev_Report(HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, 5, (uint8_t *)report);
#endif
}

static void send_extra(report_extra_t *report) {
    if (bleState != BLE_STATE_CONNECTED || bleConnHandle == GAP_CONNHANDLE_INIT) return;
    if (report->report_id == REPORT_ID_CONSUMER) {
        HidDev_Report(HID_RPT_ID_CONSUMER_IN, HID_REPORT_TYPE_INPUT, 2, (uint8_t *)&report->usage);
    } else if (report->report_id == REPORT_ID_SYSTEM) {
        uint8_t d = report->usage & 0xFF;
        HidDev_Report(HID_RPT_ID_SYSTEM_IN, HID_REPORT_TYPE_INPUT, 1, &d);
    }
}

static uint8_t ble_keyboard_leds(void) {
    return HidDev_GetKeyboardLeds();
}

// ============================================================================
// Protocol interface
// ============================================================================

const ch582_interface_t ch582_protocol_ble = {
    .ch582_common_driver.keyboard_leds = ble_keyboard_leds,
    .ch582_common_driver.send_keyboard = send_keyboard,
    .ch582_common_driver.send_nkro     = send_nkro,
    .ch582_common_driver.send_mouse    = send_mouse,
    .ch582_common_driver.send_extra    = send_extra,
    .ch582_platform_initialize  = platform_initialize,
    .ch582_protocol_setup       = protocol_setup,
    .ch582_protocol_init        = protocol_init,
    .ch582_protocol_pre_task    = protocol_pre_task,
    .ch582_protocol_post_task   = protocol_post_task,
    .ch582_platform_run         = platform_run,
    .ch582_platform_reboot      = platform_reboot,
};

// ============================================================================
// Public API
// ============================================================================

bool ble_is_connected(void) {
    return bleState == BLE_STATE_CONNECTED;
}

void ble_start_advertising(void) {
    uint8_t adv = TRUE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &adv);
}

void ble_stop_advertising(void) {
    uint8_t adv = FALSE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &adv);
}

void ble_disconnect(void) {
    if (bleConnHandle != GAP_CONNHANDLE_INIT) {
        GAPRole_TerminateLink(bleConnHandle);
    }
}

bool ble_switch_slot(uint8_t slot) {
    ble_disconnect();
    ble_start_advertising();
    return true;
}

uint8_t ble_get_current_slot(void)    { return 0; }
void    ble_clear_all_bonds(void)     { GAPBondMgr_SetParameter(GAPBOND_ERASE_ALLBONDS, 0, NULL); }
uint8_t ble_get_bond_count(void)      { return 0; }

void ble_set_power_mode(ble_power_mode_t mode) {}
ble_power_mode_t ble_get_power_mode(void) { return BLE_POWER_LOW_LATENCY; }
void ble_on_key_activity(void) {}
void ble_on_idle(void) {}
