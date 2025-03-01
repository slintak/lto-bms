#include <stdint.h>
#include <string.h>

#include <avr/io.h>
#include "uart.h"

/*******************************************************************************
 * Basic UART implementation. Only TX is enabled.
 * GPIO pins should be set before calling uart0_init().
 ******************************************************************************/

void uart0_init(uint16_t baudrate) {
    USART0.BAUD = (uint16_t)UART0_BAUD_RATE(baudrate);
    USART0.CTRLC = USART_CHSIZE_8BIT_gc;
    USART0.CTRLD = 0;
    USART0.DBGCTRL = 0;
    USART0.CTRLB = 0 << USART_MPCM_bp       /* Multi-processor Communication Mode: disabled */
                   | 0 << USART_ODME_bp     /* Open Drain Mode Enable: disabled */
                   | 0 << USART_RXEN_bp     /* Receiver Enable: disabled */
                   | USART_RXMODE_NORMAL_gc /* Normal mode */
                   | 0 << USART_SFDEN_bp    /* Start Frame Detection Enable: disabled */
                   | 1 << USART_TXEN_bp;    /* Transmitter Enable: enabled */
}

void uart0_enable_rx(void) {
    USART0.CTRLB |= USART_RXEN_bm;
}

void uart0_enable_tx(void) {
    USART0.CTRLB |= USART_TXEN_bm;
}

void uart0_disable(void) {
    USART0.CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);
}

char uart0_getc(void) {
    while (!(USART0.STATUS & USART_RXCIF_bm));
    return USART0.RXDATAL;
}

void uart0_putc(const char data) {
    while (!(USART0.STATUS & USART_DREIF_bm));
    USART0.TXDATAL = data;
}

void uart0_puts(const char *str) {
    for (int i = 0; i < strlen(str); i++) {
        uart0_putc(str[i]);
    }
}
