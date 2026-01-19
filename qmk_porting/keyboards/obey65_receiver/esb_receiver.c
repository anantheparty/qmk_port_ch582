/**
 * ESB 2.4G Receiver Implementation
 *
 * Receives keyboard data via 2.4G and forwards to USB HID
 */

#include "CH58x_common.h"
#include "CH58xBLE_LIB.H"
#include "config.h"
#include "timer.h"
#include "gpio.h"

#ifdef DEBUG_UART_ENABLE
#include "debug_uart.h"
#endif

// Import ESB types from keyboard firmware
#include "../obey65/wireless/include/esb_types.h"

// ============================================================================
// Configuration
// ============================================================================

#define DONGLE_LED_PIN      A8
#define DONGLE_BUTTON_PIN   A9

// LED modes
typedef enum {
    LED_OFF,
    LED_ON,
    LED_BLINK_SLOW,     // 1Hz - unpaired/scanning
    LED_BLINK_FAST,     // 5Hz - pairing mode
    LED_BLINK_ONCE      // Flash once - data received
} led_mode_t;

// Receiver state
typedef enum {
    RX_STATE_INIT = 0,
    RX_STATE_UNPAIRED,
    RX_STATE_PAIRING,
    RX_STATE_SCANNING,
    RX_STATE_CONNECTED
} rx_state_t;

// ============================================================================
// State Variables
// ============================================================================

static uint8_t rxTaskId = INVALID_TASK_ID;

static struct {
    rx_state_t state;
    bool paired;
    bool connected;
    uint8_t keyboard_addr[4];
    uint8_t rx_addr[4];
    uint8_t current_channel;
    uint8_t last_seq;
    uint32_t last_rx_time;
    uint32_t pairing_start_time;

    // Report buffers
    uint8_t kb_report[8];
    bool kb_report_pending;
    uint8_t mouse_report[5];
    bool mouse_report_pending;
    uint16_t consumer_report;
    bool consumer_report_pending;

    // LED
    led_mode_t led_mode;
    uint32_t led_toggle_time;
    bool led_state;
} rx_state = {
    .state = RX_STATE_INIT,
    .paired = false,
    .connected = false,
    .current_channel = 0,
    .last_seq = 0xFF,
    .last_rx_time = 0,
    .pairing_start_time = 0,
    .kb_report_pending = false,
    .mouse_report_pending = false,
    .consumer_report_pending = false,
    .led_mode = LED_OFF,
    .led_toggle_time = 0,
    .led_state = false
};

// RX buffer
static uint8_t esb_rx_buffer[ESB_MAX_PAYLOAD_LEN + 1];

// Task events
#define RX_TIMEOUT_EVT      0x0001
#define RX_PAIRING_EVT      0x0002
#define RX_PING_EVT         0x0004

// ============================================================================
// Forward Declarations
// ============================================================================

static void esb_rx_rf_callback(uint8_t sta, uint8_t rsr, uint8_t *rxBuf);
static void esb_rx_process_packet(uint8_t *data, uint8_t len);
static void esb_rx_send_ack(uint8_t seq, uint8_t led_state);
static void esb_rx_send_pairing_response(void);
static void esb_rx_start_rx(void);
static void esb_rx_set_channel(uint8_t channel_idx);
static uint16_t esb_rx_task_handler(uint8_t task_id, uint16_t events);
static void esb_rx_set_state(rx_state_t new_state);
static void led_update(void);

// ============================================================================
// RF Configuration
// ============================================================================

static rfConfig_t rx_rf_config = {
    .LLEMode = 0x01 |      // Auto mode
               (0x01 << 4) | // 2Mbps
               (0x01 << 6),  // Frequency mode
    .Channel = 0,
    .Frequency = ESB_BASE_FREQ + esb_freq_table[0] * 1000,
    .accessAddress = ESB_ACCESS_ADDRESS,
    .CRCInit = ESB_CRC_INIT,
    .rfStatusCB = esb_rx_rf_callback,
    .ChannelMap = 0,
    .RxMaxlen = ESB_MAX_PAYLOAD_LEN,
    .TxMaxlen = ESB_MAX_PAYLOAD_LEN
};

// ============================================================================
// Initialization
// ============================================================================

void esb_receiver_init(void) {
#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINT("ESB Receiver: Initializing...\n");
#endif

    // Initialize GPIO
    gpio_set_pin_output(DONGLE_LED_PIN);
    gpio_write_pin_low(DONGLE_LED_PIN);

    gpio_set_pin_input_high(DONGLE_BUTTON_PIN);

    // Register TMOS task
    rxTaskId = TMOS_ProcessEventRegister(esb_rx_task_handler);

    // Generate receiver address from chip unique ID
    uint8_t unique_id[8];
    GET_UNIQUE_ID(unique_id);
    rx_state.rx_addr[0] = 0xD0;  // Different prefix from keyboard
    rx_state.rx_addr[1] = unique_id[0];
    rx_state.rx_addr[2] = unique_id[1];
    rx_state.rx_addr[3] = unique_id[2];

    // TODO: Load pairing info from EEPROM
    // For now, assume unpaired
    rx_state.paired = false;

    // Initialize RF
    if (RF_Config(&rx_rf_config) == SUCCESS) {
#ifdef DEBUG_UART_ENABLE
        DEBUG_PRINTF("ESB Receiver: RF configured, taskId=%d\n", rxTaskId);
#endif
    } else {
#ifdef DEBUG_UART_ENABLE
        DEBUG_PRINT("ESB Receiver: RF config failed!\n");
#endif
    }

    // Check if button pressed at startup -> enter pairing
    if (gpio_read_pin(DONGLE_BUTTON_PIN) == 0) {
        esb_rx_set_state(RX_STATE_PAIRING);
    } else if (rx_state.paired) {
        esb_rx_set_state(RX_STATE_SCANNING);
    } else {
        esb_rx_set_state(RX_STATE_UNPAIRED);
    }
}

// ============================================================================
// State Management
// ============================================================================

static void esb_rx_set_state(rx_state_t new_state) {
    if (rx_state.state == new_state) return;

#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINTF("ESB Receiver: State %d -> %d\n", rx_state.state, new_state);
#endif

    rx_state.state = new_state;

    switch (new_state) {
        case RX_STATE_UNPAIRED:
            rx_state.led_mode = LED_BLINK_SLOW;
            break;

        case RX_STATE_PAIRING:
            rx_state.led_mode = LED_BLINK_FAST;
            rx_state.pairing_start_time = timer_read32();
            // Scan all channels for pairing requests
            esb_rx_set_channel(0);
            esb_rx_start_rx();
            // Pairing timeout 30 seconds
            tmos_start_task(rxTaskId, RX_PAIRING_EVT, MS1_TO_SYSTEM_TIME(30000));
            break;

        case RX_STATE_SCANNING:
            rx_state.led_mode = LED_BLINK_SLOW;
            rx_state.connected = false;
            esb_rx_set_channel(rx_state.current_channel);
            esb_rx_start_rx();
            break;

        case RX_STATE_CONNECTED:
            rx_state.led_mode = LED_ON;
            rx_state.connected = true;
            // Start ping timeout monitoring
            tmos_start_task(rxTaskId, RX_TIMEOUT_EVT, MS1_TO_SYSTEM_TIME(1000));
            break;

        default:
            break;
    }
}

// ============================================================================
// RF Operations
// ============================================================================

static void esb_rx_set_channel(uint8_t channel_idx) {
    if (channel_idx >= ESB_CHANNEL_COUNT) {
        channel_idx = 0;
    }
    rx_state.current_channel = channel_idx;

    uint32_t freq = ESB_BASE_FREQ + esb_freq_table[channel_idx] * 1000;
    RF_SetFrequency(freq);
}

static void esb_rx_start_rx(void) {
    RF_Rx(esb_rx_buffer, ESB_MAX_PAYLOAD_LEN, 0xFF, 0xFF);
}

static void esb_rx_send_ack(uint8_t seq, uint8_t led_state) {
    esb_ack_msg_t ack = {
        .type = ESB_MSG_ACK,
        .ack_seq = seq,
        .led_state = led_state
    };

    RF_Tx((uint8_t *)&ack, sizeof(ack), 0x00, 0x00);
}

static void esb_rx_send_pairing_response(void) {
    esb_pairing_rsp_t rsp = {
        .type = ESB_MSG_PAIRING_RSP,
        .channel = rx_state.current_channel
    };
    memcpy(rsp.rx_addr, rx_state.rx_addr, 4);

    RF_Tx((uint8_t *)&rsp, sizeof(rsp), 0x00, 0x00);
}

// ============================================================================
// RF Callback
// ============================================================================

static void esb_rx_rf_callback(uint8_t sta, uint8_t rsr, uint8_t *rxBuf) {
    switch (sta) {
        case TX_MODE_TX_FINISH:
            // TX complete, go back to RX
            esb_rx_start_rx();
            break;

        case TX_MODE_TX_FAIL:
            // TX failed, retry or go back to RX
            esb_rx_start_rx();
            break;

        case RX_MODE_RX_DATA:
            // Data received
            if (rxBuf && rsr > 0) {
                esb_rx_process_packet(rxBuf, rsr);
            }
            // Continue receiving
            esb_rx_start_rx();
            break;

        default:
            break;
    }
}

// ============================================================================
// Packet Processing
// ============================================================================

static void esb_rx_process_packet(uint8_t *data, uint8_t len) {
    if (len < 1) return;

    uint8_t msg_type = data[0];

    // Update activity time
    rx_state.last_rx_time = timer_read32();

    switch (msg_type) {
        case ESB_MSG_PAIRING_REQ:
            if (rx_state.state == RX_STATE_PAIRING) {
                // Handle pairing request
                esb_pairing_req_t *req = (esb_pairing_req_t *)data;
                memcpy(rx_state.keyboard_addr, req->kb_addr, 4);

#ifdef DEBUG_UART_ENABLE
                DEBUG_PRINTF("ESB Receiver: Pairing request from %02X%02X%02X%02X\n",
                    req->kb_addr[0], req->kb_addr[1], req->kb_addr[2], req->kb_addr[3]);
#endif

                // Send response
                esb_rx_send_pairing_response();

                // Mark as paired
                rx_state.paired = true;
                // TODO: Save to EEPROM

                // Switch to scanning/connected
                esb_rx_set_state(RX_STATE_SCANNING);
            }
            break;

        case ESB_MSG_KEYBOARD:
            if (rx_state.state == RX_STATE_SCANNING || rx_state.state == RX_STATE_CONNECTED) {
                esb_keyboard_msg_t *msg = (esb_keyboard_msg_t *)data;

                // Check for duplicate
                if (msg->seq != rx_state.last_seq) {
                    rx_state.last_seq = msg->seq;

                    // Build HID report
                    rx_state.kb_report[0] = msg->modifier;
                    rx_state.kb_report[1] = 0;  // Reserved
                    memcpy(&rx_state.kb_report[2], msg->keys, 6);
                    rx_state.kb_report_pending = true;

                    // Flash LED
                    rx_state.led_mode = LED_BLINK_ONCE;
                }

                // Send ACK with LED state
                extern uint8_t keyboard_led_state;
                esb_rx_send_ack(msg->seq, keyboard_led_state);

                // Ensure we're in connected state
                if (rx_state.state != RX_STATE_CONNECTED) {
                    esb_rx_set_state(RX_STATE_CONNECTED);
                }
            }
            break;

        case ESB_MSG_MOUSE:
            if (rx_state.state == RX_STATE_CONNECTED) {
                esb_mouse_msg_t *msg = (esb_mouse_msg_t *)data;

                if (msg->seq != rx_state.last_seq) {
                    rx_state.last_seq = msg->seq;

                    rx_state.mouse_report[0] = msg->buttons;
                    rx_state.mouse_report[1] = msg->x;
                    rx_state.mouse_report[2] = msg->y;
                    rx_state.mouse_report[3] = msg->wheel;
                    rx_state.mouse_report[4] = 0;
                    rx_state.mouse_report_pending = true;
                }

                extern uint8_t keyboard_led_state;
                esb_rx_send_ack(msg->seq, keyboard_led_state);
            }
            break;

        case ESB_MSG_CONSUMER:
            if (rx_state.state == RX_STATE_CONNECTED) {
                esb_consumer_msg_t *msg = (esb_consumer_msg_t *)data;

                if (msg->seq != rx_state.last_seq) {
                    rx_state.last_seq = msg->seq;
                    rx_state.consumer_report = msg->usage;
                    rx_state.consumer_report_pending = true;
                }

                extern uint8_t keyboard_led_state;
                esb_rx_send_ack(msg->seq, keyboard_led_state);
            }
            break;

        case ESB_MSG_PING:
            if (rx_state.state == RX_STATE_CONNECTED || rx_state.state == RX_STATE_SCANNING) {
                // Respond with pong
                esb_ping_msg_t pong = {
                    .type = ESB_MSG_PONG,
                    .battery_level = 0xFF  // N/A for receiver
                };
                RF_Tx((uint8_t *)&pong, sizeof(pong), 0x00, 0x00);

                if (rx_state.state != RX_STATE_CONNECTED) {
                    esb_rx_set_state(RX_STATE_CONNECTED);
                }
            }
            break;

        default:
            break;
    }
}

// ============================================================================
// Task Handler
// ============================================================================

static uint16_t esb_rx_task_handler(uint8_t task_id, uint16_t events) {
    if (events & RX_TIMEOUT_EVT) {
        // Check for connection timeout
        if (rx_state.state == RX_STATE_CONNECTED) {
            if (timer_elapsed32(rx_state.last_rx_time) > 2000) {
                // 2 seconds without data -> disconnected
#ifdef DEBUG_UART_ENABLE
                DEBUG_PRINT("ESB Receiver: Connection timeout\n");
#endif
                esb_rx_set_state(RX_STATE_SCANNING);
            } else {
                // Schedule next check
                tmos_start_task(rxTaskId, RX_TIMEOUT_EVT, MS1_TO_SYSTEM_TIME(1000));
            }
        }
        return events ^ RX_TIMEOUT_EVT;
    }

    if (events & RX_PAIRING_EVT) {
        // Pairing timeout
        if (rx_state.state == RX_STATE_PAIRING) {
#ifdef DEBUG_UART_ENABLE
            DEBUG_PRINT("ESB Receiver: Pairing timeout\n");
#endif
            if (rx_state.paired) {
                esb_rx_set_state(RX_STATE_SCANNING);
            } else {
                esb_rx_set_state(RX_STATE_UNPAIRED);
            }
        }
        return events ^ RX_PAIRING_EVT;
    }

    return 0;
}

// ============================================================================
// LED Control
// ============================================================================

static void led_update(void) {
    uint32_t now = timer_read32();
    uint32_t interval = 0;

    switch (rx_state.led_mode) {
        case LED_OFF:
            gpio_write_pin_low(DONGLE_LED_PIN);
            return;

        case LED_ON:
            gpio_write_pin_high(DONGLE_LED_PIN);
            return;

        case LED_BLINK_SLOW:
            interval = 500;
            break;

        case LED_BLINK_FAST:
            interval = 100;
            break;

        case LED_BLINK_ONCE:
            // Quick flash then back to previous state
            gpio_write_pin_high(DONGLE_LED_PIN);
            DelayMs(50);
            gpio_write_pin_low(DONGLE_LED_PIN);
            rx_state.led_mode = rx_state.connected ? LED_ON : LED_BLINK_SLOW;
            return;
    }

    if (interval > 0 && timer_elapsed32(rx_state.led_toggle_time) >= interval) {
        rx_state.led_state = !rx_state.led_state;
        if (rx_state.led_state) {
            gpio_write_pin_high(DONGLE_LED_PIN);
        } else {
            gpio_write_pin_low(DONGLE_LED_PIN);
        }
        rx_state.led_toggle_time = now;
    }
}

// ============================================================================
// Public API (called by USB interface)
// ============================================================================

bool esb_receiver_get_keyboard_report(uint8_t *report, uint8_t *len) {
    if (rx_state.kb_report_pending) {
        memcpy(report, rx_state.kb_report, 8);
        *len = 8;
        rx_state.kb_report_pending = false;
        return true;
    }
    return false;
}

bool esb_receiver_get_mouse_report(uint8_t *report, uint8_t *len) {
    if (rx_state.mouse_report_pending) {
        memcpy(report, rx_state.mouse_report, 5);
        *len = 5;
        rx_state.mouse_report_pending = false;
        return true;
    }
    return false;
}

bool esb_receiver_get_consumer_report(uint16_t *usage) {
    if (rx_state.consumer_report_pending) {
        *usage = rx_state.consumer_report;
        rx_state.consumer_report_pending = false;
        return true;
    }
    return false;
}

bool esb_receiver_is_connected(void) {
    return rx_state.connected;
}

void esb_receiver_enter_pairing(void) {
    esb_rx_set_state(RX_STATE_PAIRING);
}

// ============================================================================
// Main Loop Integration
// ============================================================================

void esb_receiver_task(void) {
    // Update LED
    led_update();

    // Check pairing button
    static bool button_pressed = false;
    bool button_state = (gpio_read_pin(DONGLE_BUTTON_PIN) == 0);

    if (button_state && !button_pressed) {
        // Button just pressed
        button_pressed = true;
        if (rx_state.state != RX_STATE_PAIRING) {
            esb_receiver_enter_pairing();
        }
    } else if (!button_state) {
        button_pressed = false;
    }
}

// ============================================================================
// USB HID Hooks (ESB_ENABLE == 2 mode)
// ============================================================================

void esb_dongle_usb_report_sent(uint8_t interface) {
    // Called when USB report was sent successfully
    // Can be used for flow control if needed
}

void esb_dongle_set_keyboard_protocol(uint8_t protocol) {
    // Called when host sets keyboard protocol (boot/report)
}

// Called from usb_interface.c to inform about USB state changes
void inform_keyboard_usb_configured(void) {
#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINT("ESB Receiver: USB configured\n");
#endif
}

void inform_keyboard_usb_suspend(bool configured) {
#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINT("ESB Receiver: USB suspended\n");
#endif
}

void inform_keyboard_usb_resume(bool configured) {
#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINT("ESB Receiver: USB resumed\n");
#endif
}
