#include "hal.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#if ENABLE_UART
#define RX_SIZE 64
#define TX_SIZE 128
static volatile uint8_t rx_head, rx_tail, tx_head, tx_tail, errors, rx_fault;
static volatile char rx[RX_SIZE], tx[TX_SIZE];
static void receive(void) {
    uint8_t status = UCSRA;
    char c = UDR;
    if (status & (_BV(FE) | _BV(DOR) | _BV(PE))) {
        errors = rx_fault = 1;
        return;
    }
    uint8_t next = (rx_head + 1) & (RX_SIZE - 1);
    if (next == rx_tail) {
        errors = rx_fault = 1;
        return;
    }
    rx[rx_head] = c;
    rx_head = next;
}
ISR(USART_RXC_vect) {
    receive();
}
ISR(USART_UDRE_vect) {
    if (tx_head == tx_tail)
        UCSRB &= ~_BV(UDRIE);
    else {
        UDR = tx[tx_tail];
        tx_tail = (tx_tail + 1) & (TX_SIZE - 1);
    }
}
#endif
void uart_init(void) {
#if ENABLE_UART
    uint16_t divisor = F_CPU / (16UL * UART_BAUD) - 1;
    UBRRH = (uint8_t)(divisor >> 8);
    UBRRL = (uint8_t)divisor;
    UCSRA = 0;
    UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
    UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);
#endif
}
void uart_rx_poll(void) {
#if ENABLE_UART
    /* Used only with interrupts masked during a DHT frame. */
    if (UCSRA & _BV(RXC))
        receive();
#endif
}
bool uart_get(char *c) {
#if ENABLE_UART
    if (rx_head == rx_tail)
        return false;
    *c = rx[rx_tail];
    rx_tail = (rx_tail + 1) & (RX_SIZE - 1);
    return true;
#else
    (void)c;
    return false;
#endif
}
bool uart_put(char c) {
#if ENABLE_UART
    uint8_t next = (tx_head + 1) & (TX_SIZE - 1);
    uint32_t end = clock_ms() + 5;
    while (next == tx_tail)
        if (due(clock_ms(), end)) {
            errors = 1;
            return false;
        }
    tx[tx_head] = c;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        tx_head = next;
        UCSRB |= _BV(UDRIE);
    }
    return true;
#else
    (void)c;
    return false;
#endif
}
void uart_text_P(PGM_P p) {
    char c;
    while ((c = pgm_read_byte(p++)))
        if (!uart_put(c))
            break;
}
void uart_uint(uint32_t n) {
    char b[10];
    uint8_t i = 0;
    do {
        b[i++] = '0' + n % 10;
        n /= 10;
    } while (n);
    while (i)
        uart_put(b[--i]);
}
void uart_fixed(uint16_t n) {
    uart_uint(n / 10);
    uart_put('.');
    uart_put('0' + n % 10);
}
uint8_t uart_errors(void) {
#if ENABLE_UART
    return errors;
#else
    return 0;
#endif
}
void uart_clear_errors(void) {
#if ENABLE_UART
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        errors = 0;
    }
#endif
}
bool uart_take_rx_fault(void) {
#if ENABLE_UART
    uint8_t v;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        v = rx_fault;
        rx_fault = 0;
    }
    return v != 0;
#else
    return false;
#endif
}
