/*
 * Obey65 BLE transport.
 *
 * OBEY65_BLE_SMOKE_TEST builds only the CH58x peripheral role and advertising
 * loop. It intentionally excludes QMK, HID over GATT, VIA, and WS2812 timers.
 */

#include "protocol_ble.h"

#include "CH58xBLE_LIB.H"
#include "ble_compat.h"
#include "config.h"

#ifndef OBEY65_BLE_SMOKE_TEST
#    include "hid_dev.h"
#    include "report.h"
#endif
#if !defined(OBEY65_BLE_SMOKE_TEST) && !defined(OBEY65_BLE_NO_QMK_TEST)
#    include "protocol_supplement.h"
#endif

#define BLE_START_DEVICE_EVT 0x0001
#define BLE_STATUS_EVT       0x0002
#define BLE_RUN_QMK_TASK_EVT 0x0004
#define BLE_RETRY_REPORT_EVT 0x0008

#define BLE_QMK_INTERVAL_MS          5
#define BLE_REPORT_RETRY_MS          10
#define BLE_AUX_REPORT_RETRY_LIMIT   100
#define BLE_KEY_REPORT_RETRY_LIMIT   1000
#define HID_ERROR_ROLLOVER_USAGE     0x01
#define BLE_MANUFACTURER_MARKER_SIZE 3

#ifndef OBEY65_BLE_SMOKE_TEST
static const uint8_t deviceName[GAP_DEVICE_NAME_LEN] = "Obey65";
#endif

static uint8_t advertData[] = {
    0x02, GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    0x07, GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'O', 'b', 'e', 'y', '6', '5',

    0x03, GAP_ADTYPE_APPEARANCE,
    LO_UINT16(GAP_APPEARE_HID_KEYBOARD),
    HI_UINT16(GAP_APPEARE_HID_KEYBOARD),

    // Type + company 0xFFFF + marker BE EF CA = six bytes after length.
    0x06, GAP_ADTYPE_MANUFACTURER_SPECIFIC,
    0xFF, 0xFF,
    0xBE, 0xEF, 0xCA,
};

static uint8_t scanRspData[] = {
    0x07, GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'O', 'b', 'e', 'y', '6', '5',

    0x05, GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
    LO_UINT16(8), HI_UINT16(8),
    LO_UINT16(8), HI_UINT16(8),

#ifndef OBEY65_BLE_SMOKE_TEST
    0x03, GAP_ADTYPE_16BIT_COMPLETE,
    LO_UINT16(HID_SERV_UUID), HI_UINT16(HID_SERV_UUID),
#endif
};

_Static_assert(sizeof(advertData) <= 31, "BLE advertising data exceeds 31 bytes");
_Static_assert(sizeof(scanRspData) <= 31, "BLE scan response exceeds 31 bytes");
_Static_assert(BLE_MANUFACTURER_MARKER_SIZE == 3, "scanner marker contract changed");

typedef enum {
    BLE_STATE_IDLE = 0,
    BLE_STATE_ADVERTISING,
    BLE_STATE_CONNECTED,
} ble_state_t;

static uint8_t              bleTaskId             = INVALID_TASK_ID;
static uint16_t             bleConnHandle         = GAP_CONNHANDLE_INIT;
static volatile ble_state_t bleState              = BLE_STATE_IDLE;
static volatile bStatus_t   bleInitStatus         = SUCCESS;
static bool                 bleAdvertisingEnabled = true;
#ifndef OBEY65_BLE_SMOKE_TEST
static bool                 bleConnectionSecure;

typedef struct {
    uint8_t id;
    uint8_t length;
    uint16_t retries;
    bool    pending;
    uint8_t data[8];
} ble_pending_report_t;

static ble_pending_report_t blePendingReports[] = {
    {.id = HID_RPT_ID_KEYBOARD_IN, .length = 8},
    {.id = HID_RPT_ID_MOUSE_IN, .length = 5},
    {.id = HID_RPT_ID_CONSUMER_IN, .length = 2},
    {.id = HID_RPT_ID_SYSTEM_IN, .length = 2},
};

static bool ble_reports_pending(void) {
    for (uint8_t i = 0; i < ARRAY_SIZE(blePendingReports); ++i) {
        if (blePendingReports[i].pending) {
            return true;
        }
    }
    return false;
}

static void ble_clear_pending_reports(void) {
    for (uint8_t i = 0; i < ARRAY_SIZE(blePendingReports); ++i) {
        blePendingReports[i].pending = false;
        blePendingReports[i].retries = 0;
    }
}

static void ble_queue_report(uint8_t id, const uint8_t *data, uint8_t length) {
    if (bleState != BLE_STATE_CONNECTED || bleConnHandle == GAP_CONNHANDLE_INIT) {
        return;
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(blePendingReports); ++i) {
        ble_pending_report_t *pending = &blePendingReports[i];
        if (pending->id != id || pending->length != length) {
            continue;
        }
        bool wasPending = pending->pending;
        tmos_memcpy(pending->data, data, length);
        pending->pending = true;
        if (!wasPending) {
            pending->retries = 0;
        }
        if (bleTaskId != INVALID_TASK_ID) {
            tmos_set_event(bleTaskId, BLE_RETRY_REPORT_EVT);
        }
        return;
    }
}

static bool ble_flush_pending_reports(void) {
    if (!bleConnectionSecure || bleState != BLE_STATE_CONNECTED || bleConnHandle == GAP_CONNHANDLE_INIT) {
        return ble_reports_pending();
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(blePendingReports); ++i) {
        ble_pending_report_t *pending = &blePendingReports[i];
        if (!pending->pending) {
            continue;
        }
        bStatus_t status = HidDev_Report(pending->id, HID_REPORT_TYPE_INPUT, pending->length, pending->data);
        if (status == SUCCESS) {
            pending->pending = false;
            pending->retries = 0;
            continue;
        }

        bool isKeyboard = pending->id == HID_RPT_ID_KEYBOARD_IN;
        bool permanent  = status == bleIncorrectMode || status == INVALIDPARAMETER;
        uint16_t retryLimit = isKeyboard ? BLE_KEY_REPORT_RETRY_LIMIT : BLE_AUX_REPORT_RETRY_LIMIT;
        if (pending->retries < retryLimit) {
            pending->retries++;
        }
        bool exhausted = pending->retries >= retryLimit;

        if (isKeyboard && (permanent || exhausted)) {
            /* A dropped release can leave a host key stuck. Disconnect instead. */
            return GAPRole_TerminateLink(bleConnHandle) != SUCCESS;
        }
        if (permanent || exhausted) {
            pending->pending = false;
            pending->retries = 0;
        }
    }
    return ble_reports_pending();
}
#endif

static void ble_record_status(bStatus_t status) {
    if (bleInitStatus == SUCCESS && status != SUCCESS) {
        bleInitStatus = status;
    }
}

#if defined(OBEY65_BLE_SMOKE_TEST) || defined(OBEY65_BLE_NO_QMK_TEST)
/* B17 is used only while QMK does not own its Caps Lock LED. */
static void ble_indicator_set(bool on) {
    if (on) {
        GPIOB_SetBits(GPIO_Pin_17);
    } else {
        GPIOB_ResetBits(GPIO_Pin_17);
    }
}

static void ble_indicator_update(void) {
    static bool error_phase;

    if (bleInitStatus != SUCCESS) {
        error_phase = !error_phase;
        ble_indicator_set(error_phase);
        return;
    }

    ble_indicator_set(bleState == BLE_STATE_ADVERTISING || bleState == BLE_STATE_CONNECTED);
}
#else
static void ble_indicator_update(void) {}
#endif

static void ble_reset_connection(void) {
#ifndef OBEY65_BLE_SMOKE_TEST
    HidDev_SetConnHandle(GAP_CONNHANDLE_INIT);
    bleConnectionSecure = false;
    ble_clear_pending_reports();
#endif
    bleConnHandle = GAP_CONNHANDLE_INIT;
}

static void ble_StateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent) {
    if (pEvent != NULL && pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT) {
        ble_reset_connection();
    }

    switch (newState & GAPROLE_STATE_ADV_MASK) {
        case GAPROLE_STARTED:
            bleState = BLE_STATE_IDLE;
            break;

        case GAPROLE_ADVERTISING:
            bleState = BLE_STATE_ADVERTISING;
            break;

        case GAPROLE_CONNECTED:
            if (pEvent != NULL && pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT) {
                bleConnHandle = pEvent->linkCmpl.connectionHandle;
#ifndef OBEY65_BLE_SMOKE_TEST
                HidDev_SetConnHandle(bleConnHandle);
                bleConnectionSecure = false;
                ble_clear_pending_reports();
#endif
                bleState = BLE_STATE_CONNECTED;
            }
            break;

        case GAPROLE_CONNECTED_ADV:
            bleState = BLE_STATE_CONNECTED;
            break;

        case GAPROLE_WAITING: {
            ble_reset_connection();
            bleState = BLE_STATE_IDLE;
            if (bleAdvertisingEnabled) {
                uint8_t advertise = TRUE;
                ble_record_status(GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(advertise), &advertise));
            }
        } break;

        case GAPROLE_ERROR:
            ble_record_status(FAILURE);
            break;

        default:
            break;
    }

    ble_indicator_update();
}

static gapRolesCBs_t blePeripheralCallbacks = {
    ble_StateNotificationCB,
    NULL,
    NULL,
};

static void ble_PasscodeCB(uint8_t *deviceAddr, uint16_t connHandle, uint8_t uiInputs, uint8_t uiOutputs) {
    (void)deviceAddr;
    (void)uiInputs;
    (void)uiOutputs;
    GAPBondMgr_PasscodeRsp(connHandle, SUCCESS, 0);
}

static void ble_PairStateCB(uint16_t connHandle, uint8_t state, uint8_t status) {
#ifndef OBEY65_BLE_SMOKE_TEST
    if (connHandle != bleConnHandle) {
        return;
    }
    if ((state == GAPBOND_PAIRING_STATE_COMPLETE || state == GAPBOND_PAIRING_STATE_BONDED) && status == SUCCESS) {
        bleConnectionSecure = true;
        HidDev_SetSecure(true);
        if (bleTaskId != INVALID_TASK_ID && ble_reports_pending()) {
            tmos_set_event(bleTaskId, BLE_RETRY_REPORT_EVT);
        }
    } else if (state == GAPBOND_PAIRING_STATE_STARTED || state == GAPBOND_PAIRING_STATE_COMPLETE || state == GAPBOND_PAIRING_STATE_BONDED) {
        bleConnectionSecure = false;
        HidDev_SetSecure(false);
    }
#else
    (void)connHandle;
    (void)state;
    (void)status;
#endif
}

static gapBondCBs_t bleBondCallbacks = {
    ble_PasscodeCB,
    ble_PairStateCB,
};

static uint16_t ble_TaskProcessEvent(uint8_t taskId, uint16_t events) {
    if (events & SYS_EVENT_MSG) {
        uint8_t *message = tmos_msg_receive(bleTaskId);
        if (message != NULL) {
            tmos_msg_deallocate(message);
        }
        return events ^ SYS_EVENT_MSG;
    }

    if (events & BLE_START_DEVICE_EVT) {
        ble_record_status(GAPRole_PeripheralStartDevice(bleTaskId, &bleBondCallbacks, &blePeripheralCallbacks));
        ble_indicator_update();
        return events ^ BLE_START_DEVICE_EVT;
    }

    if (events & BLE_STATUS_EVT) {
        ble_indicator_update();
        tmos_start_task(taskId, BLE_STATUS_EVT, MS1_TO_SYSTEM_TIME(500));
        return events ^ BLE_STATUS_EVT;
    }

#if !defined(OBEY65_BLE_SMOKE_TEST) && !defined(OBEY65_BLE_NO_QMK_TEST)
    if (events & BLE_RUN_QMK_TASK_EVT) {
        run_qmk_task();
        keyboard_check_protocol_mode();
        tmos_start_task(taskId, BLE_RUN_QMK_TASK_EVT, MS1_TO_SYSTEM_TIME(BLE_QMK_INTERVAL_MS));
        return events ^ BLE_RUN_QMK_TASK_EVT;
    }
#endif

#ifndef OBEY65_BLE_SMOKE_TEST
    if (events & BLE_RETRY_REPORT_EVT) {
        if (ble_flush_pending_reports() && bleConnectionSecure) {
            tmos_start_task(taskId, BLE_RETRY_REPORT_EVT, MS1_TO_SYSTEM_TIME(BLE_REPORT_RETRY_MS));
        }
        return events ^ BLE_RETRY_REPORT_EVT;
    }
#endif

    return 0;
}

#ifndef OBEY65_BLE_SMOKE_TEST
static void ble_configure_bonding(void) {
    uint32_t passcode    = 0;
    uint8_t  pairingMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
    uint8_t  mitm        = FALSE;
    uint8_t  io          = GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  bonding     = TRUE;

    ble_record_status(GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE, sizeof(passcode), &passcode));
    ble_record_status(GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(pairingMode), &pairingMode));
    ble_record_status(GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION, sizeof(mitm), &mitm));
    ble_record_status(GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES, sizeof(io), &io));
    ble_record_status(GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED, sizeof(bonding), &bonding));
}
#endif

static void platform_initialize(void) {
#if defined(OBEY65_BLE_SMOKE_TEST) || defined(OBEY65_BLE_NO_QMK_TEST)
    GPIOB_ModeCfg(GPIO_Pin_17, GPIO_ModeOut_PP_5mA);
    ble_indicator_set(false);
#endif

    ble_record_status(GAPRole_PeripheralInit());

    bleTaskId = TMOS_ProcessEventRegister(ble_TaskProcessEvent);
    if (bleTaskId == INVALID_TASK_ID) {
        ble_record_status(FAILURE);
        ble_indicator_update();
        return;
    }

    GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, 160); // 100 ms
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, 160); // 100 ms

    uint8_t  advertise = TRUE;
    uint16_t connMin   = 8;
    uint16_t connMax   = 8;
    ble_record_status(GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(advertise), &advertise));
    ble_record_status(GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData));
    ble_record_status(GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData));
    ble_record_status(GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(connMin), &connMin));
    ble_record_status(GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(connMax), &connMax));

#ifndef OBEY65_BLE_SMOKE_TEST
    ble_record_status(GGS_SetParameter(GGS_DEVICE_NAME_ATT, sizeof(deviceName), (void *)deviceName));
    ble_configure_bonding();
    ble_record_status(GGS_AddService(GATT_ALL_SERVICES));
    ble_record_status(GATTServApp_AddService(GATT_ALL_SERVICES));
    ble_record_status(HidDev_AddService());
#endif

    if (bleInitStatus == SUCCESS) {
        tmos_set_event(bleTaskId, BLE_START_DEVICE_EVT);
#if !defined(OBEY65_BLE_SMOKE_TEST) && !defined(OBEY65_BLE_NO_QMK_TEST)
        tmos_start_task(bleTaskId, BLE_RUN_QMK_TASK_EVT, MS1_TO_SYSTEM_TIME(BLE_QMK_INTERVAL_MS));
#endif
    }
    tmos_start_task(bleTaskId, BLE_STATUS_EVT, MS1_TO_SYSTEM_TIME(500));
}

static void ble_protocol_setup(void) {}
static void ble_protocol_init(void) {}
static void ble_protocol_pre_task(void) {}
static void ble_protocol_post_task(void) {}

static void platform_run(void) {
    TMOS_SystemProcess();
}

static void platform_reboot(void) {
    SYS_ResetExecute();
}

#ifndef OBEY65_BLE_SMOKE_TEST
static void ble_send_keyboard_payload(uint8_t mods, uint8_t reserved, const uint8_t keys[KEYBOARD_REPORT_KEYS]) {
    if (bleState != BLE_STATE_CONNECTED || bleConnHandle == GAP_CONNHANDLE_INIT) {
        return;
    }

    uint8_t payload[8] = {mods, reserved, 0, 0, 0, 0, 0, 0};
    for (uint8_t i = 0; i < KEYBOARD_REPORT_KEYS; ++i) {
        payload[i + 2] = keys[i];
    }
    ble_queue_report(HID_RPT_ID_KEYBOARD_IN, payload, sizeof(payload));
}
#endif

static void send_keyboard(report_keyboard_t *report) {
#ifndef OBEY65_BLE_SMOKE_TEST
    ble_send_keyboard_payload(report->mods, report->reserved, report->keys);
#else
    (void)report;
#endif
}

static void send_nkro(report_nkro_t *report) {
#ifndef OBEY65_BLE_SMOKE_TEST
    uint8_t keys[KEYBOARD_REPORT_KEYS] = {0};
    uint8_t count = 0;

    for (uint16_t usage = 1; usage < NKRO_REPORT_BITS * 8; ++usage) {
        if ((report->bits[usage >> 3] & (1U << (usage & 7))) == 0) {
            continue;
        }
        if (count == KEYBOARD_REPORT_KEYS) {
            for (uint8_t i = 0; i < KEYBOARD_REPORT_KEYS; ++i) {
                keys[i] = HID_ERROR_ROLLOVER_USAGE;
            }
            break;
        }
        keys[count++] = (uint8_t)usage;
    }

    ble_send_keyboard_payload(report->mods, 0, keys);
#else
    (void)report;
#endif
}

static void send_mouse(report_mouse_t *report) {
#if defined(MOUSE_ENABLE) && !defined(OBEY65_BLE_SMOKE_TEST)
    if (bleState == BLE_STATE_CONNECTED && bleConnHandle != GAP_CONNHANDLE_INIT) {
        uint8_t payload[5] = {
            report->buttons,
            (uint8_t)report->x,
            (uint8_t)report->y,
            (uint8_t)report->v,
            (uint8_t)report->h,
        };
        ble_queue_report(HID_RPT_ID_MOUSE_IN, payload, sizeof(payload));
    }
#else
    (void)report;
#endif
}

static void send_extra(report_extra_t *report) {
#ifndef OBEY65_BLE_SMOKE_TEST
    if (bleState != BLE_STATE_CONNECTED || bleConnHandle == GAP_CONNHANDLE_INIT) {
        return;
    }
    if (report->report_id == REPORT_ID_CONSUMER) {
        uint8_t usage[2] = {(uint8_t)report->usage, (uint8_t)(report->usage >> 8)};
        ble_queue_report(HID_RPT_ID_CONSUMER_IN, usage, sizeof(usage));
    } else if (report->report_id == REPORT_ID_SYSTEM) {
        uint8_t usage[2] = {(uint8_t)report->usage, (uint8_t)(report->usage >> 8)};
        ble_queue_report(HID_RPT_ID_SYSTEM_IN, usage, sizeof(usage));
    }
#else
    (void)report;
#endif
}

static uint8_t ble_keyboard_leds(void) {
#ifndef OBEY65_BLE_SMOKE_TEST
    return HidDev_GetKeyboardLeds();
#else
    return 0;
#endif
}

const ch582_interface_t ch582_protocol_ble = {
    .ch582_common_driver.keyboard_leds = ble_keyboard_leds,
    .ch582_common_driver.send_keyboard = send_keyboard,
    .ch582_common_driver.send_nkro     = send_nkro,
    .ch582_common_driver.send_mouse    = send_mouse,
    .ch582_common_driver.send_extra    = send_extra,
    .ch582_platform_initialize         = platform_initialize,
    .ch582_protocol_setup              = ble_protocol_setup,
    .ch582_protocol_init               = ble_protocol_init,
    .ch582_protocol_pre_task           = ble_protocol_pre_task,
    .ch582_protocol_post_task          = ble_protocol_post_task,
    .ch582_platform_run                = platform_run,
    .ch582_platform_reboot             = platform_reboot,
};

bool ble_is_connected(void) {
    return bleState == BLE_STATE_CONNECTED;
}

void ble_start_advertising(void) {
    bleAdvertisingEnabled = true;
    uint8_t advertise = TRUE;
    ble_record_status(GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(advertise), &advertise));
}

void ble_stop_advertising(void) {
    bleAdvertisingEnabled = false;
    uint8_t advertise = FALSE;
    ble_record_status(GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(advertise), &advertise));
}

void ble_disconnect(void) {
    if (bleConnHandle != GAP_CONNHANDLE_INIT) {
        GAPRole_TerminateLink(bleConnHandle);
    }
}

bool ble_switch_slot(uint8_t slot) {
    return ble_slot_is_supported(slot);
}

bool ble_slot_is_supported(uint8_t slot) {
    return slot == 0;
}

uint8_t ble_get_current_slot(void) {
    return 0;
}

void ble_clear_all_bonds(void) {
    GAPBondMgr_SetParameter(GAPBOND_ERASE_ALLBONDS, 0, NULL);
}

uint8_t ble_get_bond_count(void) {
    uint8_t count = 0;
    if (GAPBondMgr_GetParameter(GAPBOND_BOND_COUNT, &count) != SUCCESS) {
        return 0;
    }
    return count;
}

void ble_set_power_mode(ble_power_mode_t mode) {
    (void)mode;
}

ble_power_mode_t ble_get_power_mode(void) {
    return BLE_POWER_LOW_LATENCY;
}

void ble_on_key_activity(void) {}
void ble_on_idle(void) {}
