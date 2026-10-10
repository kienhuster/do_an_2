#include "hal.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include <util/delay.h>
static volatile uint32_t base_ms;
static volatile uint16_t fraction_us;
ISR(TIMER1_OVF_vect) {
    base_ms += 65;
    fraction_us += 536;
    if (fraction_us >= 1000) {
        fraction_us -= 1000;
        base_ms++;
    }
}
uint32_t clock_ms(void) {
    uint32_t ms;
    uint16_t us, t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ms = base_ms;
        us = fraction_us;
        t = TCNT1;
        if (TIFR & _BV(TOV1)) {
            t = TCNT1;
            ms += 65;
            us += 536;
        }
    }
    return ms + ((uint32_t)us + t) / 1000;
}
void board_init(void) {
    cli();
    /* Two consecutive writes within four CPU clocks; no interrupt between them. */
    uint8_t j = MCUCSR | _BV(JTD);
    MCUCSR = j;
    MCUCSR = j;
    DDRB &= ~0x0f;
    PORTB |= 0x0f;
    TCCR1A = 0;
    TCCR1B = _BV(CS11);
    TCNT1 = 0;
    OCR1A = 10000;
    TIFR = _BV(TOV1) | _BV(OCF1A);
    TIMSK = _BV(TOIE1) | _BV(OCIE1A);
    TCCR2 = _BV(CS21);
    TCNT2 = 0; /* 1 us counter used only for DHT timing */
#if ENABLE_BUZZER
    DDRD |= _BV(PD2);
    PORTD &= ~_BV(PD2);
#if !BUZZER_ACTIVE
    TCCR0 = _BV(WGM01) | _BV(CS01);
    OCR0 = 249;
#endif
#endif
    sei();
}
uint8_t keys_raw(void) {
    return (~PINB) & 0x0f;
}
static volatile uint8_t key_events;
ISR(TIMER1_COMPA_vect) {
    static uint8_t state, counts[4];
    OCR1A += 10000; /* 10 ms cadence, same free-running 1 MHz Timer1 */
    uint8_t pressed = keys_raw();
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t bit = _BV(i);
        if (pressed & bit) {
            if (counts[i] < 3)
                counts[i]++;
        } else if (counts[i])
            counts[i]--;
        if (counts[i] == 3 && !(state & bit)) {
            state |= bit;
            key_events |= bit;
        }
        if (!counts[i])
            state &= ~bit;
    }
}
uint8_t keys_poll(uint32_t now) {
    (void)now;
    uint8_t events;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        events = key_events;
        key_events = 0;
    }
    return events;
}
#if ENABLE_BUZZER && !BUZZER_ACTIVE
ISR(TIMER0_COMP_vect) {
    PORTD ^= _BV(PD2);
}
#endif
void buzzer_set(bool on) {
#if ENABLE_BUZZER
#if BUZZER_ACTIVE
    if (on)
        PORTD |= _BV(PD2);
    else
        PORTD &= ~_BV(PD2);
#else
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (on)
            TIMSK |= _BV(OCIE0);
        else {
            TIMSK &= ~_BV(OCIE0);
            PORTD &= ~_BV(PD2);
        }
    }
#endif
#else
    (void)on;
#endif
}
bool adc_read(uint16_t *v) {
#if ENABLE_ADC
    ADMUX = _BV(REFS0);
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1);
    ADCSRA |= _BV(ADSC);
    uint32_t deadline = clock_ms() + 5;
    while (ADCSRA & _BV(ADSC))
        if (due(clock_ms(), deadline))
            return false;
    *v = ADC;
    return true;
#else
    (void)v;
    return false;
#endif
}
void diagnostic_tick(uint32_t now, bool on) {
#if ENABLE_DIAGNOSTIC
    /* Only unused LED-bank pins; UART, LCD and buzzer pins never driven. */
    const uint8_t mask = (uint8_t)(0xff & ~(ENABLE_UART ? 3 : 0) & ~(ENABLE_LCD ? 0xe0 : 0) &
                                   ~(ENABLE_BUZZER ? 4 : 0));
    static uint8_t step;
    static uint32_t next;
    if (!on) {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            DDRD &= ~mask;
            PORTD &= ~mask;
        }
#if DIAG_LED7
        PORTC = 0xff;
        DDRC = 0xff;
#endif
        return;
    }
    DDRD |= mask;
    if (due(now, next)) {
        next = now + 300;
        step = (step + 1) % 10;
        /* Timer0 may toggle PD2 between a read and write of PORTD. */
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            PORTD = (PORTD & ~mask) | ((uint8_t)~_BV(step % 8) & mask);
        }
#if DIAG_LED7
        /* Common anode: a PC5,b PC4,c PC2,d PC1,e PC0,f PC6,g PC7,dot PC3. */
        static const uint8_t digits[10] PROGMEM = {0x88, 0xeb, 0x4c, 0x49, 0x2b,
                                                   0x19, 0x18, 0xcb, 0x08, 0x09};
        DDRC = 0xff;
        PORTC = pgm_read_byte(digits + step);
#endif
    }
#else
    (void)now;
    (void)on;
#endif
}
