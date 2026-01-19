/*
 * Obey65 2.4G Receiver - Main Integration
 *
 * This file provides the main entry point for the 2.4G USB dongle.
 * It initializes the RF receiver and forwards HID reports to USB.
 */

#include "CH58x_common.h"
#include "CH58xBLE_LIB.H"
#include "config.h"
#include "timer.h"
#include "gpio.h"
#include "quantum.h"
#include "report.h"
#include "usb_interface.h"

#include "esb_receiver.h"
#include "debug_uart.h"

// ============================================================================
// Global State
// ============================================================================

// LED state from USB host (Caps Lock, Num Lock, etc.)
uint8_t keyboard_led_state = 0;

// ============================================================================
// QMK Hooks (minimal - receiver has no matrix)
// ============================================================================

void keyboard_pre_init_kb(void) {
    // No keyboard-specific pre-init needed for receiver
}

void keyboard_post_init_kb(void) {
#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINT("Obey65 Receiver: keyboard_post_init_kb\n");
#endif
    // Initialize ESB receiver
    esb_receiver_init();
}

void housekeeping_task_kb(void) {
    // Process ESB receiver task
    esb_receiver_task();
}

// ============================================================================
// USB HID Integration
// ============================================================================

// Called by USB stack to send pending HID reports
// Returns true if a report was sent
bool receiver_send_usb_reports(void) {
    uint8_t report[8];
    uint8_t len;
    bool sent = false;

    // Check for keyboard report
    if (esb_receiver_get_keyboard_report(report, &len)) {
        hid_keyboard_send_report(KEYBOARD_MODE_BIOS, report, len);
        sent = true;
    }

    // Check for mouse report
    if (esb_receiver_get_mouse_report(report, &len)) {
        uint8_t report_to_send[6];
        report_to_send[0] = REPORT_ID_MOUSE;
        memcpy(report_to_send + 1, report, len);
        hid_exkey_send_report(report_to_send, len + 1);
        sent = true;
    }

    // Check for consumer report
    uint16_t usage;
    if (esb_receiver_get_consumer_report(&usage)) {
        report_extra_t consumer_report = {
            .report_id = REPORT_ID_CONSUMER,
            .usage = usage
        };
        hid_exkey_send_report((uint8_t *)&consumer_report, sizeof(consumer_report));
        sent = true;
    }

    return sent;
}

// Called when USB host sends LED state
void receiver_set_keyboard_leds(uint8_t led_state) {
    keyboard_led_state = led_state;
#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINTF("Obey65 Receiver: LED state = 0x%02X\n", led_state);
#endif
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(void) {
    extern void protocol_setup(void);
    extern void protocol_pre_init(void);
    extern void protocol_post_init(void);
    extern void platform_run(void);

    // Platform initialization
    platform_setup();

    // Initialize debug UART early
    DEBUG_INIT();
    DEBUG_PRINT("Obey65 Receiver starting...\n");

    // Protocol setup (USB stack)
    protocol_setup();

    // NOTE: keyboard_setup() is skipped for ESB_ENABLE == 2 (receiver mode)
    // The receiver has no physical matrix

    protocol_pre_init();

    // Minimal keyboard init
    keyboard_init();

    protocol_post_init();

    DEBUG_PRINT("Obey65 Receiver: Initialized, entering main loop\n");

    // Main loop
    for (;;) {
        platform_run();
    }
}

// ============================================================================
// Dummy Functions (required by QMK but not used by receiver)
// ============================================================================

// Matrix scanning is not needed for receiver
void matrix_init_custom(void) {}
bool matrix_scan_custom(matrix_row_t current_matrix[]) { return false; }

#ifndef NO_ACTION_LAYER
// No layers for receiver
layer_state_t layer_state_set_kb(layer_state_t state) { return state; }
#endif
