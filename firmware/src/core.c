#include "core.h"
#include <string.h>
#include <limits.h>
void settings_defaults(settings_t *s) {
    *s = (settings_t){180, 350, 300, 800, 10, 30, 2, 1};
}
bool settings_valid(const settings_t *s) {
    return s->t_low >= 0 && s->t_high <= 500 && s->t_low < s->t_high && s->h_high <= 1000 &&
           s->h_low < s->h_high && s->t_hyst >= 1 && s->t_hyst <= 100 && s->h_hyst >= 1 &&
           s->h_hyst <= 200 && (uint16_t)(s->t_high - s->t_low) > s->t_hyst &&
           s->h_high - s->h_low > s->h_hyst && s->csv_seconds >= 1 && s->csv_seconds <= 60 &&
           s->sound <= 1;
}
bool settings_set(settings_t *s, const char *key, int16_t v) {
    settings_t n = *s;
    if (!strcmp(key, "TLOW"))
        n.t_low = v;
    else if (!strcmp(key, "THIGH"))
        n.t_high = v;
    else if (!strcmp(key, "HLOW"))
        n.h_low = (uint16_t)v;
    else if (!strcmp(key, "HHIGH"))
        n.h_high = (uint16_t)v;
    else if (!strcmp(key, "THYST"))
        n.t_hyst = (uint16_t)v;
    else if (!strcmp(key, "HHYST"))
        n.h_hyst = (uint16_t)v;
    else if (!strcmp(key, "CSVSEC"))
        n.csv_seconds = (uint8_t)v;
    else if (!strcmp(key, "SOUND") && v >= 0 && v <= 1)
        n.sound = (uint8_t)v;
    else
        return false;
    /* Check before narrowing to uint8_t, too. */
    if (v < 0 || (!strcmp(key, "CSVSEC") && (v < 1 || v > 60)))
        return false;
    if (!settings_valid(&n))
        return false;
    *s = n;
    return true;
}
void stats_reset(stats_t *s) {
    memset(s, 0, sizeof(*s));
}
void stats_add(stats_t *s, uint16_t t, uint16_t h) {
    if (t > 500 || h > 1000)
        return;
    if (!s->count) {
        s->t_min = s->t_max = t;
        s->h_min = s->h_max = h;
    }
    if (t < s->t_min)
        s->t_min = t;
    if (t > s->t_max)
        s->t_max = t;
    if (h < s->h_min)
        s->h_min = h;
    if (h > s->h_max)
        s->h_max = h;
    if (s->count < STATS_LIMIT) {
        s->count++;
        s->t_sum += t;
        s->h_sum += h;
    }
    s->t[s->pos] = (int16_t)t;
    s->h[s->pos] = h;
    s->pos = (s->pos + 1) % TREND_SAMPLES;
    if (s->filled < TREND_SAMPLES)
        s->filled++;
}
int8_t stats_trend(const stats_t *s, bool humidity) {
    int16_t old = 0, recent = 0;
    if (s->filled < TREND_SAMPLES)
        return 0;
    for (uint8_t i = 0; i < TREND_SAMPLES; i++) {
        uint8_t j = (s->pos + i) % TREND_SAMPLES;
        int16_t v = humidity ? (int16_t)s->h[j] : s->t[j];
        if (i < 4)
            old += v;
        else
            recent += v;
    }
    int16_t threshold = humidity ? 80 : 20; /* means: 2 %RH / 0.5 deg C */
    int16_t d = recent - old;
    return d >= threshold ? 1 : (d <= -threshold ? -1 : 0);
}
static bool low(bool on, int16_t v, int16_t limit, uint16_t hyst) {
    return on ? v < limit + (int16_t)hyst : v <= limit;
}
static bool high(bool on, int16_t v, int16_t limit, uint16_t hyst) {
    return on ? v > limit - (int16_t)hyst : v >= limit;
}
void alarm_update(alarm_t *a, const settings_t *s, int16_t t, uint16_t h) {
    uint8_t b = 0;
    if (low(a->active & AL_T_LOW, t, s->t_low, s->t_hyst))
        b |= AL_T_LOW;
    if (high(a->active & AL_T_HIGH, t, s->t_high, s->t_hyst))
        b |= AL_T_HIGH;
    if (low(a->active & AL_H_LOW, (int16_t)h, (int16_t)s->h_low, s->h_hyst))
        b |= AL_H_LOW;
    if (high(a->active & AL_H_HIGH, (int16_t)h, (int16_t)s->h_high, s->h_hyst))
        b |= AL_H_HIGH;
    a->active = b;
    a->ack &= b; /* cleared condition can ring again on re-entry */
}
void alarm_ack(alarm_t *a) {
    a->ack = a->active;
}
uint16_t crc16(const uint8_t *p, uint8_t n) {
    uint16_t crc = 0xffff;
    while (n--) {
        crc ^= (uint16_t)*p++ << 8;
        for (uint8_t i = 0; i < 8; i++)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
    return crc;
}
static void put16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static uint16_t get16(const uint8_t *p) {
    return p[0] | ((uint16_t)p[1] << 8);
}
bool sequence_newer(uint32_t a, uint32_t b) {
    return (int32_t)(a - b) > 0;
}
void record_pack(uint8_t *r, const settings_t *s, uint32_t seq) {
    memset(r, 0, RECORD_BYTES);
    r[0] = 0xa5;
    r[1] = 1;
    for (uint8_t i = 0; i < 4; i++)
        r[2 + i] = (uint8_t)(seq >> (8 * i));
    put16(r + 6, (uint16_t)s->t_low);
    put16(r + 8, (uint16_t)s->t_high);
    put16(r + 10, s->h_low);
    put16(r + 12, s->h_high);
    put16(r + 14, s->t_hyst);
    put16(r + 16, s->h_hyst);
    r[18] = s->csv_seconds;
    r[19] = s->sound;
    put16(r + 22, crc16(r + 1, 21));
}
bool record_unpack(const uint8_t *r, settings_t *s, uint32_t *seq) {
    if (r[0] != 0xa5 || r[1] != 1 || get16(r + 22) != crc16(r + 1, 21))
        return false;
    settings_t n = {(int16_t)get16(r + 6), (int16_t)get16(r + 8), get16(r + 10), get16(r + 12),
                    get16(r + 14),         get16(r + 16),         r[18],         r[19]};
    if (!settings_valid(&n))
        return false;
    uint32_t q = 0;
    for (uint8_t i = 0; i < 4; i++)
        q |= (uint32_t)r[2 + i] << (8 * i);
    *s = n;
    *seq = q;
    return true;
}
bool rtc_valid(const rtc_time_t *t) {
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (t->year > 99 || !t->month || t->month > 12 || t->hour > 23 || t->minute > 59 ||
        t->second > 59 || !t->weekday || t->weekday > 7)
        return false;
    uint8_t max = days[t->month - 1];
    if (t->month == 2 && t->year % 4 == 0)
        max++;
    return t->day >= 1 && t->day <= max;
}
static bool bcd(uint8_t v, uint8_t *out) {
    if ((v & 15) > 9 || (v >> 4) > 9)
        return false;
    *out = (v >> 4) * 10 + (v & 15);
    return true;
}
bool rtc_decode(const uint8_t *r, rtc_time_t *t) {
    rtc_time_t n;
    if ((r[0] & 0x80) || (r[1] & 0x80) || (r[2] & 0x80) || (r[3] & 0xf8) || (r[4] & 0xc0) ||
        (r[5] & 0xe0))
        return false;
    if (!bcd(r[0], &n.second) || !bcd(r[1], &n.minute) || !bcd(r[4], &n.day) ||
        !bcd(r[5], &n.month) || !bcd(r[6], &n.year))
        return false;
    if (r[2] & 0x40) {
        uint8_t h;
        if (!bcd(r[2] & 0x1f, &h) || h < 1 || h > 12)
            return false;
        n.hour = (h % 12) + ((r[2] & 0x20) ? 12 : 0);
    } else if (!bcd(r[2], &n.hour))
        return false;
    n.weekday = r[3];
    if (!rtc_valid(&n))
        return false;
    *t = n;
    return true;
}
static uint8_t to_bcd(uint8_t v) {
    return ((v / 10) << 4) | (v % 10);
}
void rtc_encode(const rtc_time_t *t, uint8_t *r) {
    r[0] = to_bcd(t->second);
    r[1] = to_bcd(t->minute);
    r[2] = to_bcd(t->hour);
    r[3] = t->weekday;
    r[4] = to_bcd(t->day);
    r[5] = to_bcd(t->month);
    r[6] = to_bcd(t->year);
}
bool dht_decode(const uint8_t *r, uint16_t *t, uint16_t *h) {
    if ((uint8_t)(r[0] + r[1] + r[2] + r[3]) != r[4] || r[1] > 9 || r[3] > 9)
        return false;
    uint16_t tv = r[2] * 10U + r[3], hv = r[0] * 10U + r[1];
    if (tv > 500 || hv > 1000)
        return false;
    *t = tv;
    *h = hv;
    return true;
}
bool due(uint32_t now, uint32_t deadline) {
    return (int32_t)(now - deadline) >= 0;
}
bool parse_i16(const char *p, int16_t *out) {
    bool neg = false;
    uint32_t v = 0;
    if (*p == '-') {
        neg = true;
        p++;
    }
    if (!*p)
        return false;
    while (*p) {
        if (*p < '0' || *p > '9')
            return false;
        v = v * 10 + (*p++ - '0');
        if (v > (neg ? 32768UL : 32767UL))
            return false;
    }
    *out = neg ? (int16_t)(-(int32_t)v) : (int16_t)v;
    return true;
}
