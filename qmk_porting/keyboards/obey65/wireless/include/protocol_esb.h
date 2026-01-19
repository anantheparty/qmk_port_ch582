/**
 * ESB 2.4G Protocol Interface
 *
 * Provides 2.4GHz wireless connectivity for Obey65 keyboard
 */

#pragma once

#include "protocol.h"
#include "esb_types.h"
#include <stdbool.h>
#include <stdint.h>

// ESB protocol interface
extern const ch582_interface_t ch582_protocol_esb;

// ============================================================================
// Initialization
// ============================================================================

/**
 * Initialize ESB subsystem
 * Called once at startup when ESB mode is selected
 */
void esb_init(void);

/**
 * Get ESB initialization status
 */
bool esb_is_initialized(void);

// ============================================================================
// Connection Management
// ============================================================================

/**
 * Get current ESB connection state
 */
esb_state_t esb_get_state(void);

/**
 * Check if ESB is connected to receiver
 */
bool esb_is_connected(void);

/**
 * Start pairing mode
 * Broadcasts pairing request on all channels
 */
void esb_start_pairing(void);

/**
 * Stop pairing mode
 */
void esb_stop_pairing(void);

/**
 * Check if in pairing mode
 */
bool esb_is_pairing(void);

/**
 * Disconnect from receiver
 */
void esb_disconnect(void);

/**
 * Attempt reconnection to saved receiver
 */
void esb_reconnect(void);

// ============================================================================
// Configuration
// ============================================================================

/**
 * Check if pairing data is saved
 */
bool esb_has_pairing(void);

/**
 * Clear saved pairing data
 */
void esb_clear_pairing(void);

/**
 * Get current channel
 */
uint8_t esb_get_channel(void);

// ============================================================================
// Status
// ============================================================================

/**
 * Get keyboard LED state from receiver
 */
uint8_t esb_get_keyboard_leds(void);

/**
 * Register connection status callback
 */
void esb_register_status_callback(esb_status_cb_t callback);

/**
 * Register LED state callback
 */
void esb_register_led_callback(esb_led_cb_t callback);

// ============================================================================
// Power Management
// ============================================================================

/**
 * Enter low power mode (reduced polling)
 */
void esb_enter_low_power(void);

/**
 * Exit low power mode (normal polling)
 */
void esb_exit_low_power(void);
