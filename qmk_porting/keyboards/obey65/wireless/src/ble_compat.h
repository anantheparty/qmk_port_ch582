#pragma once

// Missing HID definitions
#ifndef HID_PROTOCOL_MODE_REPORT
#define HID_PROTOCOL_MODE_REPORT 0x01
#endif

#ifndef HID_REPORT_TYPE_INPUT
#define HID_REPORT_TYPE_INPUT 0x01
#endif

#ifndef HID_REPORT_TYPE_OUTPUT
#define HID_REPORT_TYPE_OUTPUT 0x02
#endif

#ifndef HID_FLAGS_REMOTE_WAKE
#define HID_FLAGS_REMOTE_WAKE 0x01
#endif

#ifndef BOOT_KEY_INPUT_UUID
#define BOOT_KEY_INPUT_UUID 0x2A22 // Boot Keyboard Input Report
#endif

// Missing GAP Role definitions
// From CH58x SDK source/examples
#ifndef GAPROLE_SLAVE_LATENCY
#define GAPROLE_SLAVE_LATENCY 0x30F
#endif

#ifndef GAPROLE_TIMEOUT_MULTIPLIER
#define GAPROLE_TIMEOUT_MULTIPLIER 0x310
#endif
