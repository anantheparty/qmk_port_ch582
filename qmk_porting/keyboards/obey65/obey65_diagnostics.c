#include "obey65_diagnostics.h"

#ifdef VIA_ENABLE

#include <stdbool.h>
#include <stdint.h>

#include "atomic_util.h"
#include "bootloader.h"
#include "protocol.h"
#include "timer.h"
#include "usb_device_state.h"
#include "usb_interface.h"

#define OBEY65_DIAG_COMMAND 0xD0
#define OBEY65_DIAG_MAGIC 0x65
#define OBEY65_DIAG_VERSION 0x01
#define OBEY65_DIAG_FRAME_SIZE 32
#define OBEY65_DIAG_HEADER_SIZE 8

enum obey65_diag_opcode {
    OBEY65_DIAG_OP_HELLO = 0x01,
    OBEY65_DIAG_OP_GET_SYSTEM = 0x02,
};

enum obey65_diag_status {
    OBEY65_DIAG_OK = 0x00,
    OBEY65_DIAG_BAD_MAGIC = 0x01,
    OBEY65_DIAG_BAD_VERSION = 0x02,
    OBEY65_DIAG_BAD_OPCODE = 0x03,
};

static uint8_t request_buffer[OBEY65_DIAG_FRAME_SIZE];
static uint8_t response_buffer[OBEY65_DIAG_FRAME_SIZE];
static volatile bool request_pending;
static volatile bool response_pending;
static volatile uint8_t dropped_requests;

static void write_u32_le(uint8_t *target, uint32_t value)
{
    target[0] = (uint8_t)value;
    target[1] = (uint8_t)(value >> 8);
    target[2] = (uint8_t)(value >> 16);
    target[3] = (uint8_t)(value >> 24);
}

static uint8_t compiled_capabilities(void)
{
    uint8_t capabilities = 0;
#ifdef USB_ENABLE
    capabilities |= (1U << 0);
#endif
#ifdef BLE_ENABLE
    capabilities |= (1U << 1);
#endif
#ifdef ESB_ENABLE
    capabilities |= (1U << 2);
#endif
#ifdef VIA_ENABLE
    capabilities |= (1U << 3);
#endif
    return capabilities;
}

static void initialize_response(const uint8_t *request, uint8_t status)
{
    memset(response_buffer, 0, sizeof(response_buffer));
    response_buffer[0] = OBEY65_DIAG_COMMAND;
    response_buffer[1] = OBEY65_DIAG_MAGIC;
    response_buffer[2] = OBEY65_DIAG_VERSION;
    response_buffer[3] = request[3];
    response_buffer[4] = request[4];
    response_buffer[5] = status;
}

static void write_profile(uint8_t *target, uint8_t capacity)
{
    static const char profile[] = OBEY65_BUILD_PROFILE;
    uint8_t i = 0;

    while (i < capacity && profile[i] != '\0') {
        target[i] = (uint8_t)profile[i];
        i++;
    }
}

static void process_request(const uint8_t *request)
{
    if (request[1] != OBEY65_DIAG_MAGIC) {
        initialize_response(request, OBEY65_DIAG_BAD_MAGIC);
        return;
    }
    if (request[2] != OBEY65_DIAG_VERSION) {
        initialize_response(request, OBEY65_DIAG_BAD_VERSION);
        return;
    }

    initialize_response(request, OBEY65_DIAG_OK);

    switch (request[4]) {
        case OBEY65_DIAG_OP_HELLO:
            response_buffer[6] = 24;
            response_buffer[8] = OBEY65_DIAG_VERSION;
            response_buffer[9] = compiled_capabilities();
            response_buffer[10] = kbd_protocol_type;
            response_buffer[11] = bootloader_boot_mode_get();
            write_u32_le(&response_buffer[12], timer_read32());
            write_profile(&response_buffer[16], 16);
            break;

        case OBEY65_DIAG_OP_GET_SYSTEM:
            response_buffer[6] = 12;
            response_buffer[8] = kbd_protocol_type;
            response_buffer[9] = bootloader_boot_mode_get();
            response_buffer[10] = (uint8_t)usb_device_state;
            response_buffer[11] = dropped_requests;
            write_u32_le(&response_buffer[12], timer_read32());
            response_buffer[16] = keyboard_protocol;
            response_buffer[17] = keyboard_idle;
            break;

        default:
            initialize_response(request, OBEY65_DIAG_BAD_OPCODE);
            break;
    }
}

bool via_command_kb(uint8_t *data, uint8_t length)
{
    if (length == 0 || data[0] != OBEY65_DIAG_COMMAND) {
        return false;
    }

    if (length != OBEY65_DIAG_FRAME_SIZE || request_pending || response_pending) {
        dropped_requests++;
        return true;
    }

    memcpy(request_buffer, data, sizeof(request_buffer));
    request_pending = true;
    return true;
}

void obey65_diagnostics_task(void)
{
    uint8_t request[OBEY65_DIAG_FRAME_SIZE];
    bool have_request = false;

    if (response_pending) {
        if (hid_qmk_raw_send_report(response_buffer, sizeof(response_buffer))) {
            response_pending = false;
        }
        return;
    }

    ATOMIC_BLOCK_FORCEON {
        if (request_pending) {
            memcpy(request, request_buffer, sizeof(request));
            request_pending = false;
            have_request = true;
        }
    }

    if (!have_request) {
        return;
    }

    process_request(request);
    response_pending = true;
}

#else

void obey65_diagnostics_task(void) {}

#endif
