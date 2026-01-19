/**
 * ESB 2.4G Protocol Implementation
 *
 * Implements Enhanced ShockBurst protocol for Obey65 keyboard
 * - RF initialization and configuration
 * - Keyboard report transmission
 * - ACK/retransmit mechanism
 * - Pairing protocol
 */

#include "protocol_esb.h"
#include "CH58xBLE_LIB.H"
#include "CH58x_common.h"
#include "config.h"
#include "report.h"
#include "timer.h"

#ifdef DEBUG_UART_ENABLE
#include "debug_uart.h"
#endif

// ============================================================================
// ESB State Management
// ============================================================================

// Task ID for ESB
static uint8_t esbTaskId = INVALID_TASK_ID;

// ESB state
static struct {
    esb_state_t state;
    esb_tx_state_t tx_state;
    uint8_t current_channel;
    uint8_t sequence;
    uint8_t led_state;
    uint8_t ping_failures;
    uint32_t last_ping_time;
    uint32_t last_activity_time;
    bool initialized;
    bool low_power_mode;
    esb_config_t config;
    esb_tx_buffer_t tx_buffer;
    esb_status_cb_t status_cb;
    esb_led_cb_t led_cb;
} esb_state = {
    .state = ESB_STATE_IDLE,
    .tx_state = ESB_TX_IDLE,
    .current_channel = 0,
    .sequence = 0,
    .led_state = 0,
    .ping_failures = 0,
    .last_ping_time = 0,
    .last_activity_time = 0,
    .initialized = false,
    .low_power_mode = false,
    .status_cb = NULL,
    .led_cb = NULL
};

// RX buffer (first byte is RSSI or packet type depending on config)
static uint8_t esb_rx_buffer[ESB_MAX_PAYLOAD_LEN + 1];

// ============================================================================
// Forward Declarations
// ============================================================================

static void esb_rf_callback(uint8_t sta, uint8_t rsr, uint8_t *rxBuf);
static void esb_send_packet(uint8_t *data, uint8_t len, uint8_t pkt_type);
static void esb_start_rx(void);
static void esb_process_rx(uint8_t *data, uint8_t len);
static void esb_handle_ack(esb_ack_msg_t *ack);
static void esb_handle_led_state(esb_led_msg_t *msg);
static void esb_hop_channel(void);
static uint16_t esb_task_handler(uint8_t task_id, uint16_t events);
static void esb_set_state(esb_state_t new_state);

// Task events
#define ESB_TX_TIMEOUT_EVT      0x0001
#define ESB_PING_EVT            0x0002
#define ESB_PAIRING_EVT         0x0004

// ============================================================================
// RF Configuration
// ============================================================================

static rfConfig_t esb_rf_config = {
    .LLEMode = 0x01 |      // Auto mode (ACK enabled)
               (0x01 << 4) | // 2Mbps PHY
               (0x01 << 6),  // Use frequency instead of channel
    .Channel = 0,
    .Frequency = ESB_BASE_FREQ + esb_freq_table[0] * 1000, // First channel
    .accessAddress = ESB_ACCESS_ADDRESS,
    .CRCInit = ESB_CRC_INIT,
    .rfStatusCB = esb_rf_callback,
    .ChannelMap = 0,       // Not using channel map
    .RxMaxlen = ESB_MAX_PAYLOAD_LEN,
    .TxMaxlen = ESB_MAX_PAYLOAD_LEN
};

// ============================================================================
// Protocol Interface Implementation
// ============================================================================

static void platform_initialize(void) {
#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINT("ESB: Initializing...\n");
#endif

    // Register TMOS task
    esbTaskId = TMOS_ProcessEventRegister(esb_task_handler);

    // Load saved config from EEPROM
    // TODO: Implement EEPROM read
    esb_state.config.magic = 0;  // Mark as not configured

    // Generate keyboard address from chip unique ID
    uint8_t unique_id[8];
    GET_UNIQUE_ID(unique_id);
    esb_state.config.kb_addr[0] = 0xE7;
    esb_state.config.kb_addr[1] = unique_id[0];
    esb_state.config.kb_addr[2] = unique_id[1];
    esb_state.config.kb_addr[3] = unique_id[2];

    // Configure RF
    if (RF_Config(&esb_rf_config) == SUCCESS) {
        esb_state.initialized = true;
#ifdef DEBUG_UART_ENABLE
        DEBUG_PRINTF("ESB: RF configured, taskId=%d\n", esbTaskId);
#endif
    } else {
#ifdef DEBUG_UART_ENABLE
        DEBUG_PRINT("ESB: RF config failed!\n");
#endif
    }
}

static void protocol_setup(void) {
    // Setup ESB protocol
}

static void protocol_init(void) {
    // Start in RX mode to listen for receiver
    if (esb_state.initialized) {
        esb_start_rx();
        esb_set_state(ESB_STATE_SCANNING);
    }
}

static void protocol_pre_task(void) {
    // ESB pre-task
}

static void protocol_post_task(void) {
    // ESB post-task - check for timeout, ping, etc.
    uint32_t now = timer_read32();

    // Check ping interval when connected
    if (esb_state.state == ESB_STATE_CONNECTED) {
        uint32_t ping_interval = esb_state.low_power_mode ?
            ESB_PING_INTERVAL_MS * 2 : ESB_PING_INTERVAL_MS;

        if (timer_elapsed32(esb_state.last_ping_time) >= ping_interval) {
            // Send ping
            esb_ping_msg_t ping = {
                .type = ESB_MSG_PING,
                .battery_level = 100  // TODO: Get actual battery level
            };
            esb_send_packet((uint8_t *)&ping, sizeof(ping), 0x00);
            esb_state.last_ping_time = now;
        }
    }
}

static void platform_run(void) {
    // ESB main loop step
    TMOS_SystemProcess();
}

static void platform_reboot(void) {
    SYS_ResetExecute();
}

// ============================================================================
// HID Report Sending
// ============================================================================

static void send_keyboard(report_keyboard_t *report) {
    if (esb_state.state != ESB_STATE_CONNECTED) {
        return;
    }

    esb_keyboard_msg_t msg = {
        .type = ESB_MSG_KEYBOARD,
        .seq = esb_state.sequence++,
        .modifier = report->mods,
        .reserved = 0
    };

    // Copy key codes
    for (int i = 0; i < 6; i++) {
        msg.keys[i] = report->keys[i];
    }

    // Store in TX buffer for retransmit if needed
    esb_state.tx_buffer.seq = msg.seq;
    esb_state.tx_buffer.payload_len = sizeof(msg);
    tmos_memcpy(esb_state.tx_buffer.payload, &msg, sizeof(msg));
    esb_state.tx_buffer.retries = 0;
    esb_state.tx_buffer.pending = true;

    esb_send_packet((uint8_t *)&msg, sizeof(msg), 0x00);
    esb_state.tx_state = ESB_TX_PENDING;
    esb_state.last_activity_time = timer_read32();

    // Start ACK timeout
    tmos_start_task(esbTaskId, ESB_TX_TIMEOUT_EVT, ESB_ACK_TIMEOUT_MS);
}

static void send_nkro(report_nkro_t *report) {
    // ESB doesn't support NKRO well, fallback to 6KRO
    // TODO: Implement if needed
}

static void send_mouse(report_mouse_t *report) {
#ifdef MOUSE_ENABLE
    if (esb_state.state != ESB_STATE_CONNECTED) {
        return;
    }

    esb_mouse_msg_t msg = {
        .type = ESB_MSG_MOUSE,
        .seq = esb_state.sequence++,
        .buttons = report->buttons,
        .x = report->x,
        .y = report->y,
        .wheel = report->v
    };

    esb_send_packet((uint8_t *)&msg, sizeof(msg), 0x00);
    esb_state.last_activity_time = timer_read32();
#endif
}

static void send_extra(report_extra_t *report) {
    if (esb_state.state != ESB_STATE_CONNECTED) {
        return;
    }

    if (report->report_id == REPORT_ID_CONSUMER) {
        esb_consumer_msg_t msg = {
            .type = ESB_MSG_CONSUMER,
            .seq = esb_state.sequence++,
            .usage = report->usage
        };
        esb_send_packet((uint8_t *)&msg, sizeof(msg), 0x00);
    }
    // System control messages handled similarly

    esb_state.last_activity_time = timer_read32();
}

static uint8_t esb_keyboard_leds_impl(void) {
    return esb_state.led_state;
}

// ============================================================================
// Protocol Interface Definition
// ============================================================================

const ch582_interface_t ch582_protocol_esb = {
    .ch582_common_driver.keyboard_leds = esb_keyboard_leds_impl,
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

// ============================================================================
// RF Callback
// ============================================================================

static void esb_rf_callback(uint8_t sta, uint8_t rsr, uint8_t *rxBuf) {
    switch (sta) {
        case TX_MODE_TX_FINISH:
            // TX successful
#ifdef DEBUG_UART_ENABLE
            DEBUG_PRINT("ESB: TX done\n");
#endif
            // In auto mode, now waiting for ACK
            break;

        case TX_MODE_TX_FAIL:
            // TX failed (TX_MODE_TX_TIMEOUT is same value)
#ifdef DEBUG_UART_ENABLE
            DEBUG_PRINT("ESB: TX failed\n");
#endif
            if (esb_state.tx_buffer.pending &&
                esb_state.tx_buffer.retries < ESB_MAX_RETRIES) {
                // Retry
                esb_state.tx_buffer.retries++;
                esb_send_packet(esb_state.tx_buffer.payload,
                               esb_state.tx_buffer.payload_len, 0x00);
            } else {
                esb_state.tx_state = ESB_TX_FAILED;
                esb_state.tx_buffer.pending = false;
            }
            break;

        case TX_MODE_RX_DATA:
            // Received ACK with data
            if ((rsr & 0x01) == 0) {  // CRC OK
                esb_process_rx(rxBuf + 1, rxBuf[0]);  // Skip RSSI byte
                esb_state.tx_state = ESB_TX_SUCCESS;
                esb_state.tx_buffer.pending = false;
                tmos_stop_task(esbTaskId, ESB_TX_TIMEOUT_EVT);
                esb_state.ping_failures = 0;  // Reset ping counter
            }
            break;

        case TX_MODE_RX_TIMEOUT:
            // No ACK received
#ifdef DEBUG_UART_ENABLE
            DEBUG_PRINT("ESB: ACK timeout\n");
#endif
            break;

        case RX_MODE_RX_DATA:
            // Received data in RX mode
            if ((rsr & 0x01) == 0) {  // CRC OK
                esb_process_rx(rxBuf + 1, rxBuf[0]);
            }
            // Restart RX
            esb_start_rx();
            break;

        default:
            break;
    }
}

// ============================================================================
// RF Operations
// ============================================================================

static void esb_send_packet(uint8_t *data, uint8_t len, uint8_t pkt_type) {
    RF_Tx(data, len, pkt_type, 0xFF);  // Broadcast type for RX
}

static void esb_start_rx(void) {
    RF_Rx(esb_rx_buffer, ESB_MAX_PAYLOAD_LEN, 0xFF, 0x00);  // Receive any type
}

static void esb_hop_channel(void) {
    esb_state.current_channel = (esb_state.current_channel + 7) % ESB_CHANNEL_COUNT;
    uint32_t freq = ESB_BASE_FREQ + esb_freq_table[esb_state.current_channel] * 1000;
    RF_SetChannel(freq);
}

// ============================================================================
// RX Processing
// ============================================================================

static void esb_process_rx(uint8_t *data, uint8_t len) {
    if (len < 1) return;

    uint8_t msg_type = data[0];

    switch (msg_type) {
        case ESB_MSG_ACK:
            if (len >= sizeof(esb_ack_msg_t)) {
                esb_handle_ack((esb_ack_msg_t *)data);
            }
            break;

        case ESB_MSG_LED_STATE:
            if (len >= sizeof(esb_led_msg_t)) {
                esb_handle_led_state((esb_led_msg_t *)data);
            }
            break;

        case ESB_MSG_PAIRING_RSP:
            if (esb_state.state == ESB_STATE_PAIRING && len >= sizeof(esb_pairing_rsp_t)) {
                esb_pairing_rsp_t *rsp = (esb_pairing_rsp_t *)data;
                // Save receiver address
                tmos_memcpy(esb_state.config.rx_addr, rsp->rx_addr, 4);
                esb_state.config.channel = rsp->channel;
                esb_state.config.paired = 1;
                esb_state.config.magic = ESB_CONFIG_MAGIC;

                // TODO: Save to EEPROM

                // Send pairing ACK
                uint8_t ack = ESB_MSG_PAIRING_ACK;
                esb_send_packet(&ack, 1, 0x00);

                esb_set_state(ESB_STATE_CONNECTED);
#ifdef DEBUG_UART_ENABLE
                DEBUG_PRINT("ESB: Paired!\n");
#endif
            }
            break;

        case ESB_MSG_PONG:
            // Heartbeat response
            esb_state.ping_failures = 0;
            break;

        default:
            break;
    }
}

static void esb_handle_ack(esb_ack_msg_t *ack) {
    // Update LED state from host
    if (ack->led_state != esb_state.led_state) {
        esb_state.led_state = ack->led_state;
        if (esb_state.led_cb) {
            esb_state.led_cb(ack->led_state);
        }
    }
}

static void esb_handle_led_state(esb_led_msg_t *msg) {
    esb_state.led_state = msg->led_state;
    if (esb_state.led_cb) {
        esb_state.led_cb(msg->led_state);
    }
}

// ============================================================================
// State Management
// ============================================================================

static void esb_set_state(esb_state_t new_state) {
    if (esb_state.state != new_state) {
        esb_state.state = new_state;
        if (esb_state.status_cb) {
            esb_state.status_cb(new_state);
        }
#ifdef DEBUG_UART_ENABLE
        static const char *state_names[] = {
            "IDLE", "SCANNING", "PAIRING", "CONNECTING", "CONNECTED", "DISCONNECTED"
        };
        DEBUG_PRINTF("ESB: State -> %s\n", state_names[new_state]);
#endif
    }
}

// ============================================================================
// Task Handler
// ============================================================================

static uint16_t esb_task_handler(uint8_t task_id, uint16_t events) {
    if (events & ESB_TX_TIMEOUT_EVT) {
        // TX timeout - retry or fail
        if (esb_state.tx_buffer.pending &&
            esb_state.tx_buffer.retries < ESB_MAX_RETRIES) {
            esb_state.tx_buffer.retries++;
            esb_send_packet(esb_state.tx_buffer.payload,
                           esb_state.tx_buffer.payload_len, 0x00);
            tmos_start_task(esbTaskId, ESB_TX_TIMEOUT_EVT, ESB_ACK_TIMEOUT_MS);
        } else {
            esb_state.tx_state = ESB_TX_FAILED;
            esb_state.tx_buffer.pending = false;
        }
        return events ^ ESB_TX_TIMEOUT_EVT;
    }

    if (events & ESB_PING_EVT) {
        // Ping timeout - increment failure count
        esb_state.ping_failures++;
        if (esb_state.ping_failures >= ESB_DISCONNECT_COUNT) {
            esb_set_state(ESB_STATE_DISCONNECTED);
        }
        return events ^ ESB_PING_EVT;
    }

    if (events & ESB_PAIRING_EVT) {
        // Continue pairing - try next channel
        esb_hop_channel();
        esb_pairing_req_t req = {
            .type = ESB_MSG_PAIRING_REQ,
            .product_id = 0x0065,  // Obey65
            .firmware_ver = 1
        };
        tmos_memcpy(req.kb_addr, esb_state.config.kb_addr, 4);
        esb_send_packet((uint8_t *)&req, sizeof(req), 0xFF);

        // Schedule next pairing attempt
        tmos_start_task(esbTaskId, ESB_PAIRING_EVT, 100);  // 100ms interval
        return events ^ ESB_PAIRING_EVT;
    }

    return 0;
}

// ============================================================================
// Public API Implementation
// ============================================================================

void esb_init(void) {
    platform_initialize();
}

bool esb_is_initialized(void) {
    return esb_state.initialized;
}

esb_state_t esb_get_state(void) {
    return esb_state.state;
}

bool esb_is_connected(void) {
    return esb_state.state == ESB_STATE_CONNECTED;
}

void esb_start_pairing(void) {
    if (!esb_state.initialized) return;

    esb_set_state(ESB_STATE_PAIRING);
    esb_state.current_channel = 0;

    // Start pairing broadcast
    tmos_start_task(esbTaskId, ESB_PAIRING_EVT, 1);

#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINT("ESB: Starting pairing\n");
#endif
}

void esb_stop_pairing(void) {
    if (esb_state.state == ESB_STATE_PAIRING) {
        tmos_stop_task(esbTaskId, ESB_PAIRING_EVT);
        RF_Shut();
        esb_set_state(ESB_STATE_IDLE);
    }
}

bool esb_is_pairing(void) {
    return esb_state.state == ESB_STATE_PAIRING;
}

void esb_disconnect(void) {
    RF_Shut();
    esb_set_state(ESB_STATE_DISCONNECTED);
}

void esb_reconnect(void) {
    if (esb_state.config.magic != ESB_CONFIG_MAGIC) {
        return;  // No saved pairing
    }

    // Set channel to saved value
    esb_state.current_channel = esb_state.config.channel;
    uint32_t freq = ESB_BASE_FREQ + esb_freq_table[esb_state.current_channel] * 1000;
    RF_SetChannel(freq);

    // Start listening
    esb_start_rx();
    esb_set_state(ESB_STATE_CONNECTING);
}

bool esb_has_pairing(void) {
    return esb_state.config.magic == ESB_CONFIG_MAGIC && esb_state.config.paired;
}

void esb_clear_pairing(void) {
    esb_state.config.magic = 0;
    esb_state.config.paired = 0;
    // TODO: Clear from EEPROM
}

uint8_t esb_get_channel(void) {
    return esb_state.current_channel;
}

uint8_t esb_get_keyboard_leds(void) {
    return esb_state.led_state;
}

void esb_register_status_callback(esb_status_cb_t callback) {
    esb_state.status_cb = callback;
}

void esb_register_led_callback(esb_led_cb_t callback) {
    esb_state.led_cb = callback;
}

void esb_enter_low_power(void) {
    esb_state.low_power_mode = true;
}

void esb_exit_low_power(void) {
    esb_state.low_power_mode = false;
}
