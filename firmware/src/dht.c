#include "hal.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#if ENABLE_DHT11
static bool level(bool high, uint8_t limit) {
    uint8_t start = TCNT2;
    while (((PINA & _BV(PA1)) != 0) != high) {
#if ENABLE_UART
        uart_rx_poll();
#endif
        if ((uint8_t)(TCNT2 - start) >= limit)
            return false;
    }
    return true;
}
#endif
void dht_init(void) {
#if ENABLE_DHT11
    DDRA &= ~_BV(PA1);
    PORTA |= _BV(PA1);
#endif
}
void dht_start(void) {
#if ENABLE_DHT11
    PORTA &= ~_BV(PA1);
    DDRA |= _BV(PA1);
#endif
}
bool dht_finish(uint16_t *t, uint16_t *h) {
#if ENABLE_DHT11
    uint8_t data[5] = {0};
    bool ok = false;
    uint8_t sr = SREG;
    cli();
    /* Open drain release; external 4.7k pull-up recommended. */
    DDRA &= ~_BV(PA1);
    PORTA |= _BV(PA1);
    _delay_us(10);
    if (!level(false, 100) || !level(true, 100) || !level(false, 100))
        goto done;
    for (uint8_t i = 0; i < 40; i++) {
        if (!level(true, 100))
            goto done;
        uint8_t start = TCNT2;
        if (!level(false, 100))
            goto done;
        data[i / 8] <<= 1;
        if ((uint8_t)(TCNT2 - start) > 45)
            data[i / 8] |= 1;
    }
    ok = dht_decode(data, t, h);
done:
#if ENABLE_UART
    uart_rx_poll();
#endif
    SREG = sr;
    return ok;
#else
    (void)t;
    (void)h;
    return false;
#endif
}
