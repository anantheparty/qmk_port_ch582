#include "hid_dev.h"
#include "CH58xBLE_LIB.H"
#include "ble_compat.h"

// ============================================================================
// Connection State
// ============================================================================
static uint16_t hidConnHandle = GAP_CONNHANDLE_INIT;

// Simple properties for GATT attributes
static uint8_t hidPropsRead = GATT_PROP_READ;
static uint8_t hidPropsWrite = GATT_PROP_WRITE_NO_RSP;
static uint8_t hidPropsReadNotify = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t hidPropsReadWriteWithoutAuth = GATT_PROP_READ | GATT_PROP_WRITE_NO_RSP;
static uint8_t hidPropsReportOutput = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;

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
    0x26, 0xFF, 0x00, // Logical Maximum (255)
    0x05, 0x07, //   Usage Page (Kbrd/Keypad)
    0x19, 0x00, //   Usage Minimum (0x00)
    0x29, 0xFF, //   Usage Maximum (0xFF)
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
    0x29, 0x08, //     Usage Maximum (0x08)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x95, 0x08, //     Report Count (8)
    0x75, 0x01, //     Report Size (1)
    0x81, 0x02, //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01, //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x09, 0x38, //     Usage (Wheel)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x03, //     Report Count (3)
    0x81, 0x06, //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x0C, //     Usage Page (Consumer)
    0x0A, 0x38, 0x02, // Usage (AC Pan)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x01, //     Report Count (1)
    0x81, 0x06, //     Input (Data,Var,Rel)
    0xC0,       //   End Collection
    0xC0,       // End Collection

    // Consumer Control Report (Media Keys)
    0x05, 0x0C, // Usage Page (Consumer Devices)
    0x09, 0x01, // Usage (Consumer Control)
    0xA1, 0x01, // Collection (Application)
    0x85, HID_RPT_ID_CONSUMER_IN, //   Report ID (3)
    0x15, 0x00, //   Logical Minimum (0)
    0x26, 0xFF, 0x03, // Logical Maximum (1023)
    0x19, 0x00, //   Usage Minimum (0)
    0x2A, 0xFF, 0x03, // Usage Maximum (1023)
    0x75, 0x10, //   Report Size (16)
    0x95, 0x01, //   Report Count (1)
    0x81, 0x00, //   Input (Data,Array,Abs)
    0xC0,       // End Collection

    // System Control Report (Power/Sleep)
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x80, // Usage (System Control)
    0xA1, 0x01, // Collection (Application)
    0x85, HID_RPT_ID_SYSTEM_IN, //   Report ID (4)
    0x19, 0x01, //   Usage Minimum
    0x2A, 0xB7, 0x00, // Usage Maximum
    0x15, 0x01, //   Logical Minimum
    0x26, 0xB7, 0x00, // Logical Maximum
    0x95, 0x01, //   Report Count (1)
    0x75, 0x10, //   Report Size (16)
    0x81, 0x00, //   Input (Data,Array,Abs)
    0xC0,       // End Collection
};

// HID Service Attributes
static const uint8_t hidServiceUUID[ATT_BT_UUID_SIZE] = { LO_UINT16(HID_SERV_UUID), HI_UINT16(HID_SERV_UUID) };
static const gattAttrType_t hidService = { ATT_BT_UUID_SIZE, hidServiceUUID };
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
static gattCharCfg_t hidReportBootKeyInClientCharCfg[GATT_MAX_NUM_CONN];
static gattCharCfg_t hidReportMouseInClientCharCfg[GATT_MAX_NUM_CONN];
static gattCharCfg_t hidReportConsumerInClientCharCfg[GATT_MAX_NUM_CONN];
static gattCharCfg_t hidReportSystemInClientCharCfg[GATT_MAX_NUM_CONN];
static uint8_t hidReportKeyIn[8];
static uint8_t hidReportKeyOut[1];
static uint8_t hidReportMouseIn[5];
static uint8_t hidReportConsumerIn[2];
static uint8_t hidReportSystemIn[2];
static uint8_t hidReportRefKeyIn[] = { HID_RPT_ID_KEYBOARD_IN, HID_REPORT_TYPE_INPUT };
static uint8_t hidReportRefKeyOut[] = { HID_RPT_ID_KEYBOARD_OUT, HID_REPORT_TYPE_OUTPUT };
static uint8_t hidReportRefMouseIn[] = { HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT };
static uint8_t hidReportRefConsumerIn[] = { HID_RPT_ID_CONSUMER_IN, HID_REPORT_TYPE_INPUT };
static uint8_t hidReportRefSystemIn[] = { HID_RPT_ID_SYSTEM_IN, HID_REPORT_TYPE_INPUT };
static bool hidConnectionSecure;

// HID Information
static const uint8_t hidInfo[] = {
    LO_UINT16(0x0111), HI_UINT16(0x0111), // bcdHID (USB HID version 1.11)
    0x00,                                 // bCountryCode
    0x00                                  // Flags
};

// GATT Attribute Table
static gattAttribute_t hidAttrTbl[] = {
    // HID Service Declaration
    { { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
      GATT_PERMIT_READ,                         /* permissions */
      0,                                        /* handle */
      (uint8_t *)&hidService                    /* pValue */
    },

    // HID Information Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsRead
    },
    // HID Information Value
    { { ATT_BT_UUID_SIZE, hidInfoUUID },
      GATT_PERMIT_ENCRYPT_READ,
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
      GATT_PERMIT_ENCRYPT_WRITE,
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
      GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE,
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
      GATT_PERMIT_ENCRYPT_READ,
      0,
      (uint8_t *)hidReportMap
    },
    // HID Report Keyboard Input Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsReadNotify
    },
    // HID Report Keyboard Input Value
    { { ATT_BT_UUID_SIZE, hidReportUUID },
      GATT_PERMIT_ENCRYPT_READ,
      0,
      hidReportKeyIn
    },
    // HID Report Keyboard Input Client Characteristic Configuration
    { { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE,
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
      &hidPropsReportOutput
    },
    // HID Report Keyboard Output Value
    { { ATT_BT_UUID_SIZE, hidReportUUID },
      GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE,
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
      GATT_PERMIT_ENCRYPT_READ,
      0,
      hidReportMouseIn
    },
    // HID Report Mouse Input Client Characteristic Configuration
    { { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE,
      0,
      (uint8_t *)&hidReportMouseInClientCharCfg
    },
    // HID Report Mouse Input Report Reference
    { { ATT_BT_UUID_SIZE, reportRefUUID },
      GATT_PERMIT_READ,
      0,
      hidReportRefMouseIn
    },

    // HID Report Consumer Input Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsReadNotify
    },
    // HID Report Consumer Input Value
    { { ATT_BT_UUID_SIZE, hidReportUUID },
      GATT_PERMIT_ENCRYPT_READ,
      0,
      hidReportConsumerIn
    },
    // HID Report Consumer Input Client Characteristic Configuration
    { { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE,
      0,
      (uint8_t *)&hidReportConsumerInClientCharCfg
    },
    // HID Report Consumer Input Report Reference
    { { ATT_BT_UUID_SIZE, reportRefUUID },
      GATT_PERMIT_READ,
      0,
      hidReportRefConsumerIn
    },

    // HID Report System Input Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsReadNotify
    },
    // HID Report System Input Value
    { { ATT_BT_UUID_SIZE, hidReportUUID },
      GATT_PERMIT_ENCRYPT_READ,
      0,
      hidReportSystemIn
    },
    // HID Report System Input Client Characteristic Configuration
    { { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE,
      0,
      (uint8_t *)&hidReportSystemInClientCharCfg
    },
    // HID Report System Input Report Reference
    { { ATT_BT_UUID_SIZE, reportRefUUID },
      GATT_PERMIT_READ,
      0,
      hidReportRefSystemIn
    },

    // Boot Keyboard Input Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsReadNotify
    },
    // Boot Keyboard Input Value
    { { ATT_BT_UUID_SIZE, hidBootKeyInputUUID },
      GATT_PERMIT_ENCRYPT_READ,
      0,
      hidReportKeyIn
    },
    // Boot Keyboard Input Client Characteristic Configuration
    { { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE,
      0,
      (uint8_t *)&hidReportBootKeyInClientCharCfg
    },

    // Boot Keyboard Output Declaration
    { { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &hidPropsReportOutput
    },
    // Boot Keyboard Output Value
    { { ATT_BT_UUID_SIZE, hidBootKeyOutputUUID },
      GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE,
      0,
      hidReportKeyOut
    },
};

// HID Service Callbacks
gattServiceCBs_t hidDevCBs = {
    HidDev_ReadAttrCB,  // Read callback function pointer
    HidDev_WriteAttrCB, // Write callback function pointer
    NULL                // Authorization callback function pointer
};

bStatus_t HidDev_AddService(void) {
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportKeyInClientCharCfg);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportBootKeyInClientCharCfg);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportMouseInClientCharCfg);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportConsumerInClientCharCfg);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportSystemInClientCharCfg);

    // Register GATT attribute list and CBs with GATT Server App
    return GATTServApp_RegisterService(hidAttrTbl,
                                       GATT_NUM_ATTRS(hidAttrTbl),
                                       GATT_MAX_ENCRYPT_KEY_SIZE,
                                       &hidDevCBs);
}

bStatus_t HidDev_Report(uint8_t id, uint8_t type, uint8_t len, uint8_t *pData) {
    // Check connection
    if (hidConnHandle == GAP_CONNHANDLE_INIT) {
        return bleNotConnected;
    }
    if (!hidConnectionSecure) {
        return bleNotReady;
    }
    if (hidProtocolMode == HID_PROTOCOL_MODE_BOOT && id != HID_RPT_ID_KEYBOARD_IN) {
        return bleIncorrectMode;
    }

    // Find the characteristic handle based on Report ID and Type
    uint16_t       handle = 0;
    gattCharCfg_t *clientConfig = NULL;
    uint8_t       *storedReport = NULL;
    uint8_t        storedLength = 0;

    if (type == HID_REPORT_TYPE_INPUT) {
        switch (id) {
            case HID_RPT_ID_KEYBOARD_IN:
                if (hidProtocolMode == HID_PROTOCOL_MODE_BOOT) {
                    handle       = hidAttrTbl[HID_BOOT_KEYBOARD_IN_IDX].handle;
                    clientConfig = hidReportBootKeyInClientCharCfg;
                } else {
                    handle       = hidAttrTbl[HID_REPORT_KEYBOARD_IN_IDX].handle;
                    clientConfig = hidReportKeyInClientCharCfg;
                }
                storedReport = hidReportKeyIn;
                storedLength = sizeof(hidReportKeyIn);
                break;
            case HID_RPT_ID_MOUSE_IN:
                handle       = hidAttrTbl[HID_REPORT_MOUSE_IN_IDX].handle;
                clientConfig = hidReportMouseInClientCharCfg;
                storedReport = hidReportMouseIn;
                storedLength = sizeof(hidReportMouseIn);
                break;
            case HID_RPT_ID_CONSUMER_IN:
                handle       = hidAttrTbl[HID_REPORT_CONSUMER_IN_IDX].handle;
                clientConfig = hidReportConsumerInClientCharCfg;
                storedReport = hidReportConsumerIn;
                storedLength = sizeof(hidReportConsumerIn);
                break;
            case HID_RPT_ID_SYSTEM_IN:
                handle       = hidAttrTbl[HID_REPORT_SYSTEM_IN_IDX].handle;
                clientConfig = hidReportSystemInClientCharCfg;
                storedReport = hidReportSystemIn;
                storedLength = sizeof(hidReportSystemIn);
                break;
            default:
                return INVALIDPARAMETER;
        }
    } else {
        return INVALIDPARAMETER;
    }

    if (handle != 0 && clientConfig != NULL) {
        if (len != storedLength) {
            return INVALIDPARAMETER;
        }
        if ((GATTServApp_ReadCharCfg(hidConnHandle, clientConfig) & GATT_CLIENT_CFG_NOTIFY) == 0) {
            return bleNotReady;
        }
        if (storedReport != NULL) {
            tmos_memcpy(storedReport, pData, MIN(len, storedLength));
        }

        attHandleValueNoti_t noti;
        noti.handle = handle;
        noti.len = len;
        noti.pValue = (uint8_t *)GATT_bm_alloc(hidConnHandle, ATT_HANDLE_VALUE_NOTI, len, NULL, 0);

        if (noti.pValue != NULL) {
            tmos_memcpy(noti.pValue, pData, len);
            bStatus_t status = GATT_Notification(hidConnHandle, &noti, FALSE);
            if (status != SUCCESS) {
                GATT_bm_free((gattMsg_t *)&noti, ATT_HANDLE_VALUE_NOTI);
            }
            return status;
        }
        return bleNoResources;
    }

    return FAILURE;
}

uint8_t HidDev_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                          uint8_t *pValue, uint16_t *pLen, uint16_t offset,
                          uint16_t maxLen, uint8_t method) {
    (void)connHandle;
    (void)method;

    bStatus_t status = SUCCESS;
    uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

    if (offset > 0 && uuid != REPORT_MAP_UUID) {
        return ATT_ERR_ATTR_NOT_LONG;
    }

    if (uuid == REPORT_UUID) {
        uint16_t reportLen = 0;
        if (pAttr->pValue == hidReportKeyIn) {
            reportLen = sizeof(hidReportKeyIn);
        } else if (pAttr->pValue == hidReportKeyOut) {
            reportLen = sizeof(hidReportKeyOut);
        } else if (pAttr->pValue == hidReportMouseIn) {
            reportLen = sizeof(hidReportMouseIn);
        } else if (pAttr->pValue == hidReportConsumerIn) {
            reportLen = sizeof(hidReportConsumerIn);
        } else if (pAttr->pValue == hidReportSystemIn) {
            reportLen = sizeof(hidReportSystemIn);
        }
        *pLen = MIN(maxLen, reportLen);
        tmos_memcpy(pValue, pAttr->pValue, *pLen);
    } else if (uuid == BOOT_KEY_INPUT_UUID || uuid == BOOT_KEY_OUTPUT_UUID) {
        uint16_t reportLen = uuid == BOOT_KEY_INPUT_UUID ? sizeof(hidReportKeyIn) : sizeof(hidReportKeyOut);
        *pLen = MIN(maxLen, reportLen);
        tmos_memcpy(pValue, pAttr->pValue, *pLen);
    } else if (uuid == REPORT_MAP_UUID) {
        if (offset >= sizeof(hidReportMap)) {
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
    } else if (uuid == GATT_REPORT_REF_UUID) {
        *pLen = MIN(maxLen, (uint16_t)2);
        tmos_memcpy(pValue, pAttr->pValue, *pLen);
    } else {
        status = ATT_ERR_ATTR_NOT_FOUND;
    }

    return status;
}

bStatus_t HidDev_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                             uint8_t *pValue, uint16_t len, uint16_t offset,
                             uint8_t method) {
    (void)method;

    if (offset > 0) {
        return ATT_ERR_ATTR_NOT_LONG;
    }

    bStatus_t status = SUCCESS;
    uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

    if (uuid == REPORT_UUID) {
        if (pAttr->pValue == hidReportKeyOut && len == sizeof(hidReportKeyOut)) {
            hidReportKeyOut[0] = pValue[0];
        } else {
            status = ATT_ERR_INVALID_VALUE_SIZE;
        }
    } else if (uuid == BOOT_KEY_OUTPUT_UUID) {
        if (len == sizeof(hidReportKeyOut)) {
            hidReportKeyOut[0] = pValue[0];
        } else {
            status = ATT_ERR_INVALID_VALUE_SIZE;
        }
    } else if (uuid == HID_CTRL_PT_UUID) {
        if (len == 1 && pValue[0] <= 1) {
            hidControlPoint = pValue[0];
        } else {
            status = ATT_ERR_INVALID_VALUE;
        }
    } else if (uuid == PROTOCOL_MODE_UUID) {
        if (len == 1 && (pValue[0] == HID_PROTOCOL_MODE_BOOT || pValue[0] == HID_PROTOCOL_MODE_REPORT)) {
            hidProtocolMode = pValue[0];
        } else {
            status = ATT_ERR_INVALID_VALUE;
        }
    } else if (uuid == GATT_CLIENT_CHAR_CFG_UUID) {
        status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len, offset, GATT_CLIENT_CFG_NOTIFY);
    } else {
        status = ATT_ERR_ATTR_NOT_FOUND;
    }

    return status;
}

// ============================================================================
// Connection and State Management
// ============================================================================

void HidDev_SetConnHandle(uint16_t connHandle) {
    if (connHandle == GAP_CONNHANDLE_INIT && hidConnHandle != GAP_CONNHANDLE_INIT) {
        GATTServApp_InitCharCfg(hidConnHandle, hidReportKeyInClientCharCfg);
        GATTServApp_InitCharCfg(hidConnHandle, hidReportBootKeyInClientCharCfg);
        GATTServApp_InitCharCfg(hidConnHandle, hidReportMouseInClientCharCfg);
        GATTServApp_InitCharCfg(hidConnHandle, hidReportConsumerInClientCharCfg);
        GATTServApp_InitCharCfg(hidConnHandle, hidReportSystemInClientCharCfg);
        hidProtocolMode = HID_PROTOCOL_MODE_REPORT;
    }
    hidConnHandle = connHandle;
    hidConnectionSecure = false;
}

void HidDev_SetSecure(bool secure) {
    hidConnectionSecure = secure;
}

uint8_t HidDev_GetKeyboardLeds(void) {
    return hidReportKeyOut[0];
}
