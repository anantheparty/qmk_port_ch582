#include "protocol_ble.h"
#include "CH58xBLE_LIB.H"
#include "config.h"
#include "hid_dev.h"
#include "ble_compat.h"

// Task ID for BLE
static uint8_t bleTaskId = INVALID_TASK_ID;

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8_t scanRspData[] = {
    // complete name
    0x07,   // length of this data
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'O', 'b', 'e', 'y', '6', '5',

    // connection interval range
    0x05,   // length of this data
    GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
    LO_UINT16(80),   // 100ms
    HI_UINT16(80),
    LO_UINT16(800),  // 1s
    HI_UINT16(800),

    // Tx power level
    0x02,   // length of this data
    GAP_ADTYPE_POWER_LEVEL,
    0       // 0dBm
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static uint8_t advertData[] = {
    // Flags; this sets the device to use limited discoverable
    // mode (advertises for 30 seconds at a time) instead of general
    // discoverable mode (advertises indefinitely)
    0x02,   // length of this data
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    // Service UUIDs
    0x03,   // length of this data
    GAP_ADTYPE_16BIT_MORE,
    LO_UINT16(HID_SERV_UUID),
    HI_UINT16(HID_SERV_UUID),
};

// GAP Role Callbacks
static gapRolesCBs_t simpleBLEPeripheral_PeripheralCBs = {
    NULL, // Profile State Change Callbacks
    NULL, // When a valid RSSI is read from controller (not used by application)
    NULL
};

// Bond Manager Callbacks
static gapBondCBs_t simpleBLEPeripheral_BondMgrCBs = {
    NULL, // Passcode callback (not used by application)
    NULL  // Pairing / Bonding state Callback (not used by application)
};

static uint16_t BLE_Task_ProcessEvent(uint8_t task_id, uint16_t events);

static void platform_initialize() {
    // Register TMOS task
    bleTaskId = TMOS_ProcessEventRegister(BLE_Task_ProcessEvent);
    
    // Setup GAP Peripheral Role Profile
    {
        uint8_t  initial_advertising_enable = TRUE;
        uint16_t desired_min_interval = 80;
        uint16_t desired_max_interval = 800;
        uint16_t desired_slave_latency = 0;
        uint16_t desired_conn_timeout = 1000;

        // Set the GAP Role Parameters
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
        GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);
        GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16_t), &desired_min_interval);
        GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16_t), &desired_max_interval);
        GAPRole_SetParameter(GAPROLE_SLAVE_LATENCY, sizeof(uint16_t), &desired_slave_latency);
        GAPRole_SetParameter(GAPROLE_TIMEOUT_MULTIPLIER, sizeof(uint16_t), &desired_conn_timeout);
    }

    // Set the GAP Characteristics
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, sizeof("Obey65"), "Obey65");

    // Setup the GAP Bond Manager
    {
        uint32_t passkey = 0; // passkey "000000"
        uint8_t  pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
        uint8_t  mitm = TRUE;
        uint8_t  ioCap = GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT;
        uint8_t  bonding = TRUE;

        GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE, sizeof(uint32_t), &passkey);
        GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(uint8_t), &pairMode);
        GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION, sizeof(uint8_t), &mitm);
        GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
        GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED, sizeof(uint8_t), &bonding);
    }

    // Initialize GATT attributes
    GGS_AddService(GATT_ALL_SERVICES);         // GAP
    GATTServApp_AddService(GATT_ALL_SERVICES); // GATT attributes
    HidDev_AddService();                       // HID Service
    
    // Start the Device
    GAPRole_PeripheralStartDevice(bleTaskId, &simpleBLEPeripheral_BondMgrCBs, &simpleBLEPeripheral_PeripheralCBs);
}

static uint16_t BLE_Task_ProcessEvent(uint8_t task_id, uint16_t events) {
    if (events & SYS_EVENT_MSG) {
        uint8_t *pMsg;

        if ((pMsg = tmos_msg_receive(bleTaskId)) != NULL) {
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }

        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    return 0;
}

static void protocol_setup() {
    // Setup BLE protocol
}

static void protocol_init() {
    // Init BLE hardware
    GAPRole_PeripheralInit();
}

static void protocol_pre_task() {
    // BLE pre-task
}

static void protocol_post_task() {
    // BLE post-task
}

static void platform_run() {
    // BLE main loop step
    TMOS_SystemProcess();
}

static void platform_reboot() {
    // Handle reboot
    SYS_ResetExecute();
}

static void send_keyboard(report_keyboard_t *report) {
    // Send keyboard report over BLE via HID Service
    HidDev_Report(HID_RPT_ID_KEYBOARD_IN, HID_REPORT_TYPE_INPUT, 8, (uint8_t *)report);
}

static void send_nkro(report_nkro_t *report) {
    // TODO: Send NKRO report over BLE
}

static void send_mouse(report_mouse_t *report) {
    // Send mouse report over BLE
#ifdef MOUSE_ENABLE
    HidDev_Report(HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, 5, (uint8_t *)report);
#endif
}

static void send_extra(report_extra_t *report) {
    // TODO: Send extra report over BLE
}

static uint8_t ble_keyboard_leds(void) {
    // TODO: Return LED state from BLE
    return 0;
}

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

