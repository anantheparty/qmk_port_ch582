/*
 * Debug UART for Obey65
 * Uses UART2 with alternate pins: PB22 (RX), PB23 (TX)
 */

#include "debug_uart.h"

#ifdef DEBUG_UART_ENABLE

#include "CH58x_common.h"
#include "printf.h"
#include <stdarg.h>

// UART2 FIFO size
#define UART2_FIFO_SIZE 8

// Baudrate for debug output
#define DEBUG_UART_BAUD 115200

static bool debug_initialized = false;

void debug_uart_init(void) {
    if (debug_initialized) return;

    // Enable UART2 alternate pins (PB22/PB23)
    // R8_PIN_ALTERNATE |= RB_PIN_UART2;
    GPIOPinRemap(ENABLE, RB_PIN_UART2);

    // Configure PB23 as TX output
    GPIOB_ModeCfg(GPIO_Pin_23, GPIO_ModeOut_PP_5mA);
    GPIOB_SetBits(GPIO_Pin_23);  // TX idle high

    // Configure PB22 as RX input (optional, for future use)
    GPIOB_ModeCfg(GPIO_Pin_22, GPIO_ModeIN_PU);

    // Initialize UART2
    UART2_DefInit();
    UART2_BaudRateCfg(DEBUG_UART_BAUD);

    debug_initialized = true;

    // Print startup banner
    debug_uart_puts("\r\n");
    debug_uart_puts("=================================\r\n");
    debug_uart_puts("  Obey65 Debug Console\r\n");
    debug_uart_puts("  UART2 @ 115200 baud\r\n");
    debug_uart_puts("=================================\r\n");
}

void debug_uart_putc(char c) {
    if (!debug_initialized) return;

    // Wait for TX FIFO space
    while (R8_UART2_TFC >= UART2_FIFO_SIZE) {
        __nop();
    }
    R8_UART2_THR = c;
}

void debug_uart_puts(const char *str) {
    if (!debug_initialized) return;

    while (*str) {
        debug_uart_putc(*str++);
    }
}

// Printf output callback for fctprintf
static void debug_putchar_callback(char c, void *arg) {
    (void)arg;
    debug_uart_putc(c);
}

void debug_uart_printf(const char *fmt, ...) {
    if (!debug_initialized) return;

    va_list args;
    va_start(args, fmt);
    // Use fctprintf from printf.h for custom output
    char buffer[128];
    vsnprintf_(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    debug_uart_puts(buffer);
}

void debug_matrix_event(uint8_t row, uint8_t col, bool pressed) {
    if (!debug_initialized) return;

    debug_uart_printf("[MTX] R%d C%d %s\r\n",
                      row, col,
                      pressed ? "DOWN" : "UP");
}

void debug_hid_report(const uint8_t *report, uint8_t len) {
    if (!debug_initialized) return;

    debug_uart_puts("[HID] ");
    for (uint8_t i = 0; i < len; i++) {
        debug_uart_printf("%02X ", report[i]);
    }
    debug_uart_puts("\r\n");
}

// Implement _putchar for printf_ support (optional global printf)
void _putchar(char character) {
    debug_uart_putc(character);
}

#endif // DEBUG_UART_ENABLE
