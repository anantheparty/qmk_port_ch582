#include "hid_dev.h"
#include "CH58xBLE_LIB.H"
#include "ble_compat.h"

// Simple properties for GATT attributes
static uint8_t hidPropsRead = GATT_PROP_READ;
static uint8_t hidPropsWrite = GATT_PROP_WRITE_NO_RSP;
static uint8_t hidPropsReadNotify = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t hidPropsReadWriteWithoutAuth = GATT_PROP_READ | GATT_PROP_WRITE_NO_RSP;

// HID Report Map
static const uint8_t hidReportMap[] = {
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06, // Usage (Keyboard)
    0xA1, 0x01, // Collection (Application)
    0x85, HID_RPT_ID_KEYBOARD_IN, //   Report ID (1)
    0x05, 0x07, //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0, //   Usage Minimum (0xE0)
    0x29, 0xE7, //   Usage Maximum (0xE7)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01, //   Report Count (1)
    0x75, 0x08, //   Report Size (8)
    0x81, 0x03, //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05, //   Report Count (5)
    0x75, 0x01, //   Report Size (1)
    0x05, 0x08, //   Usage Page (LEDs)
    0x19, 0x01, //   Usage Minimum (Num Lock)
    0x29, 0x05, //   Usage Maximum (Kana)
    0x91, 0x02, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01, //   Report Count (1)
    0x75, 0x03, //   Report Size (3)
    0x91, 0x03, //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06, //   Report Count (6)
    0x75, 0x08, //   Report Size (8)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x65, //   Logical Maximum (101)
    0x05, 0x07, //   Usage Page (Kbrd/Keypad)
    0x19, 0x00, //   Usage Minimum (0x00)
    0x29, 0x65, //   Usage Maximum (0x65)
    0x81, 0x00, //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,       // End Collection

    // Mouse Report
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
    0x85, HID_RPT_ID_MOUSE_IN, //   Report ID (2)
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)
    0x05, 0x09, //     Usage Page (Button)
    0x19, 0x01, //     Usage Minimum (0x01)
    0x29, 0x05, //     Usage Maximum (0x05)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x95, 0x05, //     Report Count (5)
    0x75, 0x01, //     Report Size (1)
    0x81, 0x02, //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01, //     Report Count (1)
    0x75, 0x03, //     Report Size (3)
    0x81, 0x03, //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01, //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x09, 0x38, //     Usage (Wheel)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x03, //     Report Count (3)
    0x81, 0x06, //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,       //   End Collection
    0xC0,       // End Collection
};

// HID Service Attributes
static const uint8_t hidServiceUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(HID_SERV_UUID), HI_UINT16(HID_SERV_UUID) };
// includeUUID is extern in CH58xBLE_LIB.H
static const uint8_t hidInfoUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(HID_INFORMATION_UUID), HI_UINT16(HID_INFORMATION_UUID) };
static const uint8_t hidControlPointUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(HID_CTRL_PT_UUID), HI_UINT16(HID_CTRL_PT_UUID) };
static const uint8_t hidProtocolModeUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(PROTOCOL_MODE_UUID), HI_UINT16(PROTOCOL_MODE_UUID) };
static const uint8_t hidReportMapUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(REPORT_MAP_UUID), HI_UINT16(REPORT_MAP_UUID) };
static const uint8_t hidReportUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(REPORT_UUID), HI_UINT16(REPORT_UUID) };
static const uint8_t hidBootKeyInputUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(BOOT_KEY_INPUT_UUID), HI_UINT16(BOOT_KEY_INPUT_UUID) }; // Correct UUID needed
static const uint8_t hidBootKeyOutputUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(BOOT_KEY_OUTPUT_UUID), HI_UINT16(BOOT_KEY_OUTPUT_UUID) };
// clientCharCfgUUID is extern in CH58xBLE_LIB.H
// reportRefUUID is extern in CH58xBLE_LIB.H
// extReportRefUUID is extern in CH58xBLE_LIB.H

// Attribute Variables
static uint8_t hidProtocolMode = HID_PROTOCOL_MODE_REPORT;
static uint8_t hidControlPoint;
static gattCharCfg_t hidReportKeyInClientCharCfg[GATT_MAX_NUM_CONN];
static gattCharCfg_t hidReportMouseInClientCharCfg[GATT_MAX_NUM_CONN];
static uint8_t hidReportKeyIn[8];
static uint8_t hidReportKeyOut[1];
static uint8_t hidReportMouseIn[5];
static uint8_t hidReportRefKeyIn[] = { HID_RPT_ID_KEYBOARD_IN, HID_REPORT_TYPE_INPUT };
static uint8_t hidReportRefKeyOut[] = { HID_RPT_ID_KEYBOARD_OUT, HID_REPORT_TYPE_OUTPUT };
static uint8_t hidReportRefMouseIn[] = { HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT };

// HID Information
static const uint8_t hidInfo[] = {
    LO_UINT16(0x0111), HI_UINT16(0x0111), // bcdHID (USB HID version 1.11)
    0x00,                                 // bCountryCode
    HID_FLAGS_REMOTE_WAKE                 // Flags
};

// GATT Attribute Table
static gattAttribute_t hidAttrTbl[] = {
    // HID Service Declaration
    { { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
      GATT_PERMIT_READ,                         /* permissions */
      0,                                        /* handle */
      (uint8_t *)&hidServiceUUID                /* pValue */
    },

    // Included Service Declaration (Battery Service - placeholder)
    { { ATT_BT_UUID_SIZE, includeUUID },
      GATT_PERMIT_READ,
      0,
      (uint8_t *)&hidServiceUUID // Should point to Battery Service handle
    },

    // HID Information Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsRead
    },
    // HID Information Value
    { { ATT_BT_UUID_SIZE, hidInfoUUID },
      GATT_PERMIT_READ,
      0,
      (uint8_t *)hidInfo
    },

    // HID Control Point Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsWrite
    },
    // HID Control Point Value
    { { ATT_BT_UUID_SIZE, hidControlPointUUID },
      GATT_PERMIT_WRITE,
      0,
      &hidControlPoint
    },

    // HID Protocol Mode Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsReadWriteWithoutAuth
    },
    // HID Protocol Mode Value
    { { ATT_BT_UUID_SIZE, hidProtocolModeUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE,
      0,
      &hidProtocolMode
    },

    // HID Report Map Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsRead
    },
    // HID Report Map Value
    { { ATT_BT_UUID_SIZE, hidReportMapUUID },
      GATT_PERMIT_READ,
      0,
      (uint8_t *)hidReportMap
    },
    // HID Report Map External Report Reference
    { { ATT_BT_UUID_SIZE, extReportRefUUID },
      GATT_PERMIT_READ,
      0,
      (uint8_t *)&hidServiceUUID // Battery Service UUID placeholder
    },

    // HID Report Keyboard Input Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsReadNotify
    },
    // HID Report Keyboard Input Value
    { { ATT_BT_UUID_SIZE, hidReportUUID },
      GATT_PERMIT_READ,
      0,
      hidReportKeyIn
    },
    // HID Report Keyboard Input Client Characteristic Configuration
    { { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE,
      0,
      (uint8_t *)&hidReportKeyInClientCharCfg
    },
    // HID Report Keyboard Input Report Reference
    { { ATT_BT_UUID_SIZE, reportRefUUID },
      GATT_PERMIT_READ,
      0,
      hidReportRefKeyIn
    },

    // HID Report Keyboard Output Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsReadWriteWithoutAuth
    },
    // HID Report Keyboard Output Value
    { { ATT_BT_UUID_SIZE, hidReportUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE,
      0,
      hidReportKeyOut
    },
    // HID Report Keyboard Output Report Reference
    { { ATT_BT_UUID_SIZE, reportRefUUID },
      GATT_PERMIT_READ,
      0,
      hidReportRefKeyOut
    },

    // HID Report Mouse Input Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsReadNotify
    },
    // HID Report Mouse Input Value
    { { ATT_BT_UUID_SIZE, hidReportUUID },
      GATT_PERMIT_READ,
      0,
      hidReportMouseIn
    },
    // HID Report Mouse Input Client Characteristic Configuration
    { { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE,
      0,
      (uint8_t *)&hidReportMouseInClientCharCfg
    },
    // HID Report Mouse Input Report Reference
    { { ATT_BT_UUID_SIZE, reportRefUUID },
      GATT_PERMIT_READ,
      0,
      hidReportRefMouseIn
    },
};

// HID Service Callbacks
gattServiceCBs_t hidDevCBs = {
    HidDev_ReadAttrCB,  // Read callback function pointer
    HidDev_WriteAttrCB, // Write callback function pointer
    NULL                // Authorization callback function pointer
};

bStatus_t HidDev_AddService(void) {
    // Register GATT attribute list and CBs with GATT Server App
    return GATTServApp_RegisterService(hidAttrTbl,
                                       GATT_NUM_ATTRS(hidAttrTbl),
                                       GATT_MAX_ENCRYPT_KEY_SIZE,
                                       &hidDevCBs);
}

bStatus_t HidDev_Report(uint8_t id, uint8_t type, uint8_t len, uint8_t *pData) {
    // Find the characteristic handle based on Report ID and Type
    uint16_t handle = 0;
    gattCharCfg_t *pCharCfg = NULL;

    if (type == HID_REPORT_TYPE_INPUT) {
        if (id == HID_RPT_ID_KEYBOARD_IN) {
            handle = hidAttrTbl[HID_REPORT_KEYBOARD_IN_IDX].handle;
            pCharCfg = hidReportKeyInClientCharCfg;
        } else if (id == HID_RPT_ID_MOUSE_IN) {
            handle = hidAttrTbl[HID_REPORT_MOUSE_IN_IDX].handle;
            pCharCfg = hidReportMouseInClientCharCfg;
        }
    }

    if (handle != 0) {
        // Update value
        GATTServApp_WriteCharCfg(0, pCharCfg, 0); // Dummy call to check cfg?
        // Actually, we should use GATTServApp_ProcessCharCfg or sending notification manually
        // For now, simple notification:
        
        attHandleValueNoti_t noti;
        noti.handle = handle;
        noti.len = len;
        noti.pValue = (uint8_t *)GATT_bm_alloc(0, ATT_HANDLE_VALUE_NOTI, len, NULL, 0);
        
        if (noti.pValue != NULL) {
            tmos_memcpy(noti.pValue, pData, len);
            return GATT_Notification(0, &noti, FALSE);
        }
    }
    
    return FAILURE;
}

uint8_t HidDev_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                          uint8_t *pValue, uint16_t *pLen, uint16_t offset,
                          uint16_t maxLen, uint8_t method) {
    bStatus_t status = SUCCESS;
    uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

    if (uuid == REPORT_UUID) {
        // Read report
        *pLen = 1; // Dummy length
        pValue[0] = 0;
    } else if (uuid == REPORT_MAP_UUID) {
        // Read report map
        if (offset > sizeof(hidReportMap)) {
            return ATT_ERR_INVALID_OFFSET;
        }
        *pLen = MIN(maxLen, sizeof(hidReportMap) - offset);
        tmos_memcpy(pValue, &hidReportMap[offset], *pLen);
    } else if (uuid == HID_INFORMATION_UUID) {
        *pLen = sizeof(hidInfo);
        tmos_memcpy(pValue, pAttr->pValue, sizeof(hidInfo));
    } else if (uuid == PROTOCOL_MODE_UUID) {
        *pLen = 1;
        pValue[0] = hidProtocolMode;
    } else {
        status = ATT_ERR_ATTR_NOT_FOUND;
    }

    return status;
}

bStatus_t HidDev_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                             uint8_t *pValue, uint16_t len, uint16_t offset,
                             uint8_t method) {
    bStatus_t status = SUCCESS;
    uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

    if (uuid == REPORT_UUID) {
        // Write report (LEDs)
        if (len > 0) {
            hidReportKeyOut[0] = pValue[0];
            // Update global LED state if necessary
        }
    } else if (uuid == HID_CTRL_PT_UUID) {
        hidControlPoint = pValue[0];
    } else if (uuid == PROTOCOL_MODE_UUID) {
        hidProtocolMode = pValue[0];
    } else if (uuid == GATT_CLIENT_CHAR_CFG_UUID) {
        status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len, offset, GATT_CLIENT_CFG_NOTIFY);
    } else {
        status = ATT_ERR_ATTR_NOT_FOUND;
    }

    return status;
}
