/*
 * Debug UART for Obey65 Receiver
 * Uses UART2 default pins: PA6 (RX), PA7 (TX)
 */

#include "debug_uart.h"

#ifdef DEBUG_UART_ENABLE

#include "CH58x_common.h"
#include "printf.h"
#include <stdarg.h>

#define UART2_FIFO_SIZE 8
#define DEBUG_UART_BAUD 115200

static bool debug_initialized = false;

void debug_uart_init(void) {
    if (debug_initialized) return;

    // Use default UART2 pins (PA6/PA7)
    GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);
    GPIOA_SetBits(GPIO_Pin_7);  // TX idle high
    GPIOA_ModeCfg(GPIO_Pin_6, GPIO_ModeIN_PU);

    UART2_DefInit();
    UART2_BaudRateCfg(DEBUG_UART_BAUD);

    debug_initialized = true;

    debug_uart_puts("\r\n");
    debug_uart_puts("=================================\r\n");
    debug_uart_puts("  Obey65 Receiver Debug Console\r\n");
    debug_uart_puts("  UART2 @ 115200 baud\r\n");
    debug_uart_puts("=================================\r\n");
}

void debug_uart_putc(char c) {
    if (!debug_initialized) return;
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

void debug_uart_printf(const char *fmt, ...) {
    if (!debug_initialized) return;

    va_list args;
    va_start(args, fmt);
    char buffer[128];
    vsnprintf_(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    debug_uart_puts(buffer);
}

#endif // DEBUG_UART_ENABLE
