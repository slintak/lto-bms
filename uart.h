#ifndef USART_BASIC_H_INCLUDED
#define USART_BASIC_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static char debug_tmp[128] __attribute__((unused));
#define LOG_INIT(b) uart0_init(b)
#define LOG(...) do { snprintf(debug_tmp, sizeof(debug_tmp), __VA_ARGS__); uart0_puts(debug_tmp); } while(0)

#ifdef DEBUG_ENABLE
#define DEBUG(...) do { snprintf(debug_tmp, sizeof(debug_tmp), __VA_ARGS__); uart0_puts(debug_tmp); } while(0)
#else
#define DEBUG(...)
#endif

/* Normal Mode, Baud register value */
#define UART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

void uart0_init(uint16_t baudrate);

void uart0_enable_rx(void);

void uart0_enable_tx(void);

void uart0_disable(void);

char uart0_getc(void);

void uart0_putc(const char data);

void uart0_puts(const char *str);

#endif
