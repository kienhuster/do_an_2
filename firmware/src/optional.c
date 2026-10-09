#include "hal.h"
bool rtc_read(rtc_time_t *t) {
#if ENABLE_RTC
    uint8_t r[7], status;
    if (!i2c_regs(0x68, 0, r, 7) || !i2c_regs(0x68, 0x0f, &status, 1) || (status & 0x80))
        return false;
    return rtc_decode(r, t);
#else
    (void)t;
    return false;
#endif
}
bool rtc_set(const rtc_time_t *t) {
#if ENABLE_RTC
    uint8_t r[8] = {0}, status, control;
    if (!rtc_valid(t))
        return false;
    rtc_encode(t, r + 1);
    if (!i2c_write(0x68, r, 8) || !i2c_regs(0x68, 0x0f, &status, 1))
        return false;
    r[0] = 0x0f;
    r[1] = status & ~0x80;
    if (!i2c_write(0x68, r, 2) || !i2c_regs(0x68, 0x0e, &control, 1))
        return false;
    r[0] = 0x0e;
    r[1] = control & ~0x80; /* EOSC=0: keep oscillator running on battery */
    return i2c_write(0x68, r, 2);
#else
    (void)t;
    return false;
#endif
}
bool light_start(void) {
#if ENABLE_BH1750
    uint8_t c = 0x01;
    if (!i2c_write(BH1750_ADDRESS, &c, 1))
        return false;
    c = 0x07;
    if (!i2c_write(BH1750_ADDRESS, &c, 1))
        return false;
    c = 0x10;
    return i2c_write(BH1750_ADDRESS, &c, 1); /* continuous H-resolution, 180ms max */
#else
    return false;
#endif
}
bool light_read(uint32_t *lux) {
#if ENABLE_BH1750
    uint8_t r[2];
    if (!i2c_read(BH1750_ADDRESS, r, 2))
        return false;
    *lux = (((uint32_t)r[0] << 8) | r[1]) * 5 / 6;
    return true;
#else
    (void)lux;
    return false;
#endif
}
