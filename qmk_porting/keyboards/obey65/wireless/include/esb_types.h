/**
 * ESB 2.4G Protocol Data Types
 *
 * Defines data structures for Enhanced ShockBurst protocol
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// RF Configuration
// ============================================================================

#define ESB_BASE_FREQ           2400000     // Base frequency 2400MHz (in kHz)
#define ESB_CHANNEL_COUNT       16          // Number of available channels
#define ESB_DATA_RATE_2M        0x01        // 2Mbps data rate
#define ESB_TX_POWER            0x1D        // LL_TX_POWEER_4_DBM

// Access address (must match between keyboard and receiver)
#define ESB_ACCESS_ADDRESS      0xE7E7E7E7

// CRC initial value
#define ESB_CRC_INIT           0x555555

// Frequency hopping table (avoiding WiFi channels 1, 6, 11)
static const uint8_t esb_freq_table[ESB_CHANNEL_COUNT] = {
    2, 26, 50, 74,   // Avoid WiFi Ch 1, 6, 11
    5, 29, 53, 77,
    8, 32, 56, 80,
    11, 35, 59, 83
};

// ============================================================================
// Timing Configuration
// ============================================================================

#define ESB_MAX_RETRIES         3           // Maximum retransmit count
#define ESB_RETRY_DELAY_US      500         // Retransmit delay (microseconds)
#define ESB_ACK_TIMEOUT_MS      10          // ACK timeout (milliseconds)
#define ESB_PING_INTERVAL_MS    500         // Heartbeat interval
#define ESB_DISCONNECT_COUNT    3           // Ping failures before disconnect

// ============================================================================
// Message Types
// ============================================================================

typedef enum {
    // Pairing messages
    ESB_MSG_PAIRING_REQ     = 0x01,         // Pairing request
    ESB_MSG_PAIRING_RSP     = 0x02,         // Pairing response
    ESB_MSG_PAIRING_ACK     = 0x03,         // Pairing acknowledgment

    // HID reports
    ESB_MSG_KEYBOARD        = 0x10,         // Keyboard report
    ESB_MSG_MOUSE           = 0x11,         // Mouse report
    ESB_MSG_CONSUMER        = 0x12,         // Consumer control report
    ESB_MSG_SYSTEM          = 0x13,         // System control report

    // Control messages
    ESB_MSG_ACK             = 0x80,         // General ACK
    ESB_MSG_NACK            = 0x81,         // Negative ACK

    // Heartbeat
    ESB_MSG_PING            = 0xF0,         // Ping
    ESB_MSG_PONG            = 0xF1,         // Pong

    // LED feedback
    ESB_MSG_LED_STATE       = 0xE0,         // LED state from host
} esb_msg_type_t;

// ============================================================================
// ESB State Machine
// ============================================================================

typedef enum {
    ESB_STATE_IDLE = 0,                     // Not active
    ESB_STATE_SCANNING,                     // Scanning for receiver
    ESB_STATE_PAIRING,                      // Pairing in progress
    ESB_STATE_CONNECTING,                   // Attempting connection
    ESB_STATE_CONNECTED,                    // Connected and active
    ESB_STATE_DISCONNECTED                  // Disconnected
} esb_state_t;

typedef enum {
    ESB_TX_IDLE = 0,                        // No pending TX
    ESB_TX_PENDING,                         // TX in progress
    ESB_TX_WAIT_ACK,                        // Waiting for ACK
    ESB_TX_SUCCESS,                         // TX successful
    ESB_TX_FAILED                           // TX failed after retries
} esb_tx_state_t;

// ============================================================================
// Message Structures
// ============================================================================

// Generic message header
typedef struct {
    uint8_t type;                           // Message type
    uint8_t seq;                            // Sequence number
} esb_msg_header_t;

// Pairing request
typedef struct {
    uint8_t type;                           // ESB_MSG_PAIRING_REQ
    uint8_t kb_addr[4];                     // Keyboard address
    uint16_t product_id;                    // Product ID
    uint8_t firmware_ver;                   // Firmware version
} esb_pairing_req_t;

// Pairing response
typedef struct {
    uint8_t type;                           // ESB_MSG_PAIRING_RSP
    uint8_t rx_addr[4];                     // Receiver address
    uint8_t channel;                        // Assigned channel
} esb_pairing_rsp_t;

// Keyboard report message
typedef struct {
    uint8_t type;                           // ESB_MSG_KEYBOARD
    uint8_t seq;                            // Sequence number
    uint8_t modifier;                       // Modifier keys
    uint8_t reserved;                       // Reserved
    uint8_t keys[6];                        // Key codes
} esb_keyboard_msg_t;

// Mouse report message
typedef struct {
    uint8_t type;                           // ESB_MSG_MOUSE
    uint8_t seq;                            // Sequence number
    uint8_t buttons;                        // Button states
    int8_t x;                               // X movement
    int8_t y;                               // Y movement
    int8_t wheel;                           // Scroll wheel
} esb_mouse_msg_t;

// Consumer control message
typedef struct {
    uint8_t type;                           // ESB_MSG_CONSUMER
    uint8_t seq;                            // Sequence number
    uint16_t usage;                         // Consumer usage code
} esb_consumer_msg_t;

// ACK message
typedef struct {
    uint8_t type;                           // ESB_MSG_ACK
    uint8_t ack_seq;                        // Acknowledged sequence
    uint8_t led_state;                      // Current LED state from host
} esb_ack_msg_t;

// Ping message
typedef struct {
    uint8_t type;                           // ESB_MSG_PING
    uint8_t battery_level;                  // Battery percentage
} esb_ping_msg_t;

// LED state message (from receiver)
typedef struct {
    uint8_t type;                           // ESB_MSG_LED_STATE
    uint8_t led_state;                      // LED state bits
} esb_led_msg_t;

// ============================================================================
// TX Buffer
// ============================================================================

#define ESB_MAX_PAYLOAD_LEN     32

typedef struct {
    uint8_t seq;                            // Sequence number
    uint8_t retries;                        // Retry count
    uint32_t send_time;                     // Timestamp of last send
    uint8_t payload[ESB_MAX_PAYLOAD_LEN];   // Payload data
    uint8_t payload_len;                    // Payload length
    bool pending;                           // Has pending data
} esb_tx_buffer_t;

// ============================================================================
// Configuration Storage
// ============================================================================

#define ESB_CONFIG_MAGIC        0xE5B12340

typedef struct {
    uint32_t magic;                         // Magic number for validation
    uint8_t kb_addr[4];                     // Keyboard address
    uint8_t rx_addr[4];                     // Receiver address
    uint8_t channel;                        // Current channel index
    uint8_t paired;                         // Pairing status
    uint16_t crc;                           // Checksum
} esb_config_t;

// ============================================================================
// Callback Types
// ============================================================================

// Connection status callback
typedef void (*esb_status_cb_t)(esb_state_t state);

// LED state callback (from receiver)
typedef void (*esb_led_cb_t)(uint8_t led_state);
