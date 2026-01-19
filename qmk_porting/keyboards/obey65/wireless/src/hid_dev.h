#pragma once

#include "CH58xBLE_LIB.H"
#include "protocol.h"

// HID Service UUID
#define HID_SERV_UUID           0x1812

// HID Report IDs
#define HID_RPT_ID_KEYBOARD_IN  1
#define HID_RPT_ID_MOUSE_IN     2
#define HID_RPT_ID_CONSUMER_IN  3
#define HID_RPT_ID_SYSTEM_IN    4
#define HID_RPT_ID_KEYBOARD_OUT 1

// Attribute Handles - HID Service
enum {
    HID_SERVICE_IDX,
    HID_INCL_SERVICE_IDX,
    HID_INFO_DECL_IDX,
    HID_INFO_IDX,
    HID_CONTROL_POINT_DECL_IDX,
    HID_CONTROL_POINT_IDX,
    HID_PROTOCOL_MODE_DECL_IDX,
    HID_PROTOCOL_MODE_IDX,
    HID_REPORT_MAP_DECL_IDX,
    HID_REPORT_MAP_IDX,
    HID_REPORT_MAP_EXT_IDX,
    HID_REPORT_KEYBOARD_IN_DECL_IDX,
    HID_REPORT_KEYBOARD_IN_IDX,
    HID_REPORT_KEYBOARD_IN_CCCD_IDX,
    HID_REPORT_KEYBOARD_IN_REF_IDX,
    HID_REPORT_KEYBOARD_OUT_DECL_IDX,
    HID_REPORT_KEYBOARD_OUT_IDX,
    HID_REPORT_KEYBOARD_OUT_REF_IDX,
    HID_REPORT_MOUSE_IN_DECL_IDX,
    HID_REPORT_MOUSE_IN_IDX,
    HID_REPORT_MOUSE_IN_CCCD_IDX,
    HID_REPORT_MOUSE_IN_REF_IDX,
    HID_REPORT_CONSUMER_IN_DECL_IDX,
    HID_REPORT_CONSUMER_IN_IDX,
    HID_REPORT_CONSUMER_IN_CCCD_IDX,
    HID_REPORT_CONSUMER_IN_REF_IDX,
    HID_REPORT_SYSTEM_IN_DECL_IDX,
    HID_REPORT_SYSTEM_IN_IDX,
    HID_REPORT_SYSTEM_IN_CCCD_IDX,
    HID_REPORT_SYSTEM_IN_REF_IDX,
    HID_BOOT_KEYBOARD_IN_DECL_IDX,
    HID_BOOT_KEYBOARD_IN_IDX,
    HID_BOOT_KEYBOARD_IN_CCCD_IDX,
    HID_BOOT_KEYBOARD_OUT_DECL_IDX,
    HID_BOOT_KEYBOARD_OUT_IDX,
    HID_SERVICE_ATTR_COUNT
};

// Function prototypes
bStatus_t HidDev_AddService(void);
bStatus_t HidDev_Report(uint8_t id, uint8_t type, uint8_t len, uint8_t *pData);
uint8_t HidDev_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                          uint8_t *pValue, uint16_t *pLen, uint16_t offset,
                          uint16_t maxLen, uint8_t method);
bStatus_t HidDev_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                             uint8_t *pValue, uint16_t len, uint16_t offset,
                             uint8_t method);

// Connection and state management
void HidDev_SetConnHandle(uint16_t connHandle);
uint8_t HidDev_GetKeyboardLeds(void);

