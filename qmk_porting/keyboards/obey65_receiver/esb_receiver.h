/**
 * ESB 2.4G Receiver Interface
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Initialize ESB receiver
void esb_receiver_init(void);

// Main task (call from main loop)
void esb_receiver_task(void);

// Get pending reports
bool esb_receiver_get_keyboard_report(uint8_t *report, uint8_t *len);
bool esb_receiver_get_mouse_report(uint8_t *report, uint8_t *len);
bool esb_receiver_get_consumer_report(uint16_t *usage);

// Status
bool esb_receiver_is_connected(void);

// Enter pairing mode
void esb_receiver_enter_pairing(void);

// USB hooks (ESB_ENABLE == 2)
void esb_dongle_usb_report_sent(uint8_t interface);
void esb_dongle_set_keyboard_protocol(uint8_t protocol);
void inform_keyboard_usb_configured(void);
void inform_keyboard_usb_suspend(bool configured);
void inform_keyboard_usb_resume(bool configured);
