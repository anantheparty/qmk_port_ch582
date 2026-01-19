/*
 * Debug UART for Obey65
 * Uses UART2 with alternate pins: PB22 (RX), PB23 (TX)
 *
 * Enable with DEBUG_UART_ENABLE in rules.cmake
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef DEBUG_UART_ENABLE

// Initialize debug UART
void debug_uart_init(void);

// Send a single byte
void debug_uart_putc(char c);

// Send a string
void debug_uart_puts(const char *str);

// Send formatted output (uses printf internally)
void debug_uart_printf(const char *fmt, ...);

// Debug macros
#define DEBUG_INIT()        debug_uart_init()
#define DEBUG_PRINT(s)      debug_uart_puts(s)
#define DEBUG_PRINTF(...)   debug_uart_printf(__VA_ARGS__)

// Matrix debug - print key press/release events
void debug_matrix_event(uint8_t row, uint8_t col, bool pressed);

// HID debug - print outgoing HID reports
void debug_hid_report(const uint8_t *report, uint8_t len);

#else

// No-op when debug is disabled
#define DEBUG_INIT()        ((void)0)
#define DEBUG_PRINT(s)      ((void)0)
#define DEBUG_PRINTF(...)   ((void)0)

static inline void debug_matrix_event(uint8_t row, uint8_t col, bool pressed) { (void)row; (void)col; (void)pressed; }
static inline void debug_hid_report(const uint8_t *report, uint8_t len) { (void)report; (void)len; }

#endif
