#ifndef CORE_H
#define CORE_H
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
enum { AL_T_LOW = 1, AL_T_HIGH = 2, AL_H_LOW = 4, AL_H_HIGH = 8 };
enum {
    ERR_DHT = 1,
    ERR_RTC = 2,
    ERR_LIGHT = 4,
    ERR_ADC = 8,
    ERR_UART = 16,
    ERR_EEPROM = 32,
    ERR_WATCHDOG = 64
};
typedef struct {
    int16_t t_low, t_high;
    uint16_t h_low, h_high, t_hyst, h_hyst;
    uint8_t csv_seconds, sound;
} settings_t;
typedef struct {
    uint32_t count, t_sum, h_sum;
    uint16_t t_min, t_max, h_min, h_max;
    int16_t t[TREND_SAMPLES];
    uint16_t h[TREND_SAMPLES];
    uint8_t pos, filled;
} stats_t;
typedef struct {
    uint8_t active, ack;
} alarm_t;
typedef struct {
    uint8_t year, month, day, hour, minute, second, weekday;
} rtc_time_t;
#define SETTINGS_BYTES 14
#define RECORD_BYTES 24
void settings_defaults(settings_t *s);
bool settings_valid(const settings_t *s);
bool settings_set(settings_t *s, const char *key, int16_t value);
void stats_reset(stats_t *s);
void stats_add(stats_t *s, uint16_t t10, uint16_t h10);
int8_t stats_trend(const stats_t *s, bool humidity);
void alarm_update(alarm_t *a, const settings_t *s, int16_t t10, uint16_t h10);
void alarm_ack(alarm_t *a);
uint16_t crc16(const uint8_t *p, uint8_t n);
void record_pack(uint8_t *r, const settings_t *s, uint32_t sequence);
bool record_unpack(const uint8_t *r, settings_t *s, uint32_t *sequence);
bool sequence_newer(uint32_t a, uint32_t b);
bool rtc_valid(const rtc_time_t *t);
bool rtc_decode(const uint8_t *r, rtc_time_t *t);
void rtc_encode(const rtc_time_t *t, uint8_t *r);
bool dht_decode(const uint8_t *r, uint16_t *t10, uint16_t *h10);
bool due(uint32_t now, uint32_t deadline);
bool parse_i16(const char *text, int16_t *out);
#endif
