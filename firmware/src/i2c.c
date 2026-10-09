#include "hal.h"
#include <avr/io.h>
#include <util/twi.h>
#include <util/delay.h>
#if ENABLE_RTC || ENABLE_BH1750
static bool wait_twi(void) {
    for (uint16_t i = 0; i < 1000; i++) {
        if (TWCR & _BV(TWINT))
            return true;
        _delay_us(10);
    }
    return false;
}
static bool start(uint8_t address) {
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
    if (!wait_twi() || (TW_STATUS != TW_START && TW_STATUS != TW_REP_START))
        return false;
    TWDR = address;
    TWCR = _BV(TWINT) | _BV(TWEN);
    if (!wait_twi())
        return false;
    return TW_STATUS == ((address & 1) ? TW_MR_SLA_ACK : TW_MT_SLA_ACK);
}
static bool put(uint8_t v) {
    TWDR = v;
    TWCR = _BV(TWINT) | _BV(TWEN);
    return wait_twi() && TW_STATUS == TW_MT_DATA_ACK;
}
static bool get(uint8_t *v, bool ack) {
    TWCR = _BV(TWINT) | _BV(TWEN) | (ack ? _BV(TWEA) : 0);
    if (!wait_twi() || TW_STATUS != (ack ? TW_MR_DATA_ACK : TW_MR_DATA_NACK))
        return false;
    *v = TWDR;
    return true;
}
static void recover(void) {
    TWCR = 0;
    PORTC &= ~3;
    DDRC &= ~3;
    /* Open-drain clock pulses then STOP. Never drive a high level. */
    for (uint8_t i = 0; i < 9; i++) {
        DDRC |= _BV(PC0);
        _delay_us(5);
        DDRC &= ~_BV(PC0);
        _delay_us(5);
    }
    DDRC |= _BV(PC1);
    _delay_us(5);
    DDRC &= ~_BV(PC0);
    _delay_us(5);
    DDRC &= ~_BV(PC1);
    _delay_us(5);
    TWCR = _BV(TWEN);
}
static bool stop(void) {
    TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);
    for (uint16_t i = 0; i < 1000; i++) {
        if (!(TWCR & _BV(TWSTO)))
            return true;
        _delay_us(10);
    }
    recover();
    return false;
}
#endif
void i2c_init(void) {
#if ENABLE_RTC || ENABLE_BH1750
    DDRC &= ~3;
    PORTC &= ~3; /* use external pullups to the appropriate bus voltage */
    TWSR = 0;
    TWBR = (F_CPU / 100000UL - 16) / 2;
    TWCR = _BV(TWEN);
#endif
}
bool i2c_write(uint8_t addr, const uint8_t *p, uint8_t n) {
#if ENABLE_RTC || ENABLE_BH1750
    bool ok = start(addr << 1);
    while (ok && n--)
        ok = put(*p++);
    if (!stop())
        ok = false;
    if (!ok)
        recover();
    return ok;
#else
    (void)addr;
    (void)p;
    (void)n;
    return false;
#endif
}
bool i2c_read(uint8_t addr, uint8_t *p, uint8_t n) {
#if ENABLE_RTC || ENABLE_BH1750
    bool ok = start((addr << 1) | 1);
    while (ok && n) {
        n--;
        ok = get(p++, n != 0);
    }
    if (!stop())
        ok = false;
    if (!ok)
        recover();
    return ok;
#else
    (void)addr;
    (void)p;
    (void)n;
    return false;
#endif
}
bool i2c_regs(uint8_t addr, uint8_t reg, uint8_t *p, uint8_t n) {
#if ENABLE_RTC || ENABLE_BH1750
    bool ok = start(addr << 1) && put(reg) && start((addr << 1) | 1);
    while (ok && n) {
        n--;
        ok = get(p++, n != 0);
    }
    if (!stop())
        ok = false;
    if (!ok)
        recover();
    return ok;
#else
    (void)addr;
    (void)reg;
    (void)p;
    (void)n;
    return false;
#endif
}
