/*
 * Debug UART for Obey65 Receiver
 * Uses UART2 for debug output
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef DEBUG_UART_ENABLE

void debug_uart_init(void);
void debug_uart_putc(char c);
void debug_uart_puts(const char *str);
void debug_uart_printf(const char *fmt, ...);

#define DEBUG_INIT()        debug_uart_init()
#define DEBUG_PRINT(s)      debug_uart_puts(s)
#define DEBUG_PRINTF(...)   debug_uart_printf(__VA_ARGS__)

#else

#define DEBUG_INIT()        ((void)0)
#define DEBUG_PRINT(s)      ((void)0)
#define DEBUG_PRINTF(...)   ((void)0)

#endif
