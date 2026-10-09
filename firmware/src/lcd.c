#include "hal.h"
#include <avr/io.h>
#include <util/delay.h>
#if ENABLE_LCD
static void nibble(uint8_t v) {
    PORTC = (PORTC & 0x0f) | (v & 0xf0);
    _delay_us(1);
    PORTD |= _BV(PD7);
    _delay_us(1);
    PORTD &= ~_BV(PD7);
    _delay_us(1);
}
static void send(uint8_t v, bool rs) {
    if (rs)
        PORTD |= _BV(PD6);
    else
        PORTD &= ~_BV(PD6);
    PORTD &= ~_BV(PD5);
    nibble(v);
    nibble(v << 4);
    _delay_us(50);
}
#endif
void lcd_init(void) {
#if ENABLE_LCD
    DDRC |= 0xf0;
    DDRD |= _BV(PD5) | _BV(PD6) | _BV(PD7);
    PORTD &= ~0xe0;
    _delay_ms(40);
    nibble(0x30);
    _delay_ms(5);
    nibble(0x30);
    _delay_us(150);
    nibble(0x30);
    _delay_us(150);
    nibble(0x20);
    _delay_us(150);
    send(0x28, false);
    send(0x0c, false);
    send(0x06, false);
    send(0x01, false);
    _delay_ms(2);
#endif
}
void lcd_line(uint8_t row, const char *text) {
#if ENABLE_LCD
    send(row ? 0xc0 : 0x80, false);
    bool end = false;
    for (uint8_t i = 0; i < 16; i++) {
        if (!end && !*text)
            end = true;
        send(end ? ' ' : (uint8_t)*text++, true);
    }
#else
    (void)row;
    (void)text;
#endif
}
