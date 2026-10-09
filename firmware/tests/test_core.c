#include "core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static unsigned checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        checks++;                                                                                  \
        if (!(x)) {                                                                                \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                           \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static void test_settings(void) {
    settings_t s, old;
    settings_defaults(&s);
    CHECK(settings_valid(&s));
    old = s;
    CHECK(!settings_set(&s, "TLOW", 350));
    CHECK(!memcmp(&s, &old, sizeof(s)));
    CHECK(!settings_set(&s, "THIGH", 501));
    CHECK(!settings_set(&s, "HLOW", -1));
    CHECK(!settings_set(&s, "CSVSEC", 257));
    CHECK(!settings_set(&s, "SOUND", 256));
    CHECK(!settings_set(&s, "HHYST", 0));
    CHECK(!settings_set(&s, "UNKNOWN", 1));
    CHECK(settings_set(&s, "THIGH", 400));
    CHECK(s.t_high == 400);
    CHECK(settings_set(&s, "SOUND", 0));
    CHECK(s.sound == 0);
    int16_t n;
    CHECK(parse_i16("32767", &n) && n == 32767);
    CHECK(parse_i16("-32768", &n) && n == -32768);
    CHECK(!parse_i16("32768", &n));
    CHECK(!parse_i16("999999999999999999", &n));
    CHECK(!parse_i16("12x", &n));
    CHECK(!parse_i16("", &n));
    CHECK(!parse_i16("-", &n));
}
static void test_alarms(void) {
    settings_t s;
    settings_defaults(&s);
    alarm_t a = {0};
    alarm_update(&a, &s, 250, 600);
    CHECK(a.active == 0);
    alarm_update(&a, &s, 350, 800);
    CHECK(a.active == (AL_T_HIGH | AL_H_HIGH));
    alarm_ack(&a);
    CHECK(a.ack == a.active);
    alarm_update(&a, &s, 345, 790);
    CHECK(a.active == 10 && a.ack == 10);
    alarm_update(&a, &s, 340, 770);
    CHECK(!a.active && !a.ack);
    alarm_update(&a, &s, 350, 800);
    CHECK(a.active == 10 && !a.ack);
    alarm_ack(&a);
    alarm_update(&a, &s, 180, 300);
    CHECK(a.active == 5 && !a.ack);
    alarm_update(&a, &s, 189, 329);
    CHECK(a.active == 5);
    alarm_update(&a, &s, 190, 330);
    CHECK(!a.active);
}
static void test_stats(void) {
    stats_t s;
    stats_reset(&s);
    CHECK(!s.count && stats_trend(&s, false) == 0);
    for (uint8_t i = 0; i < 8; i++)
        stats_add(&s, 200 + i * 10, 500 + i * 20);
    CHECK(s.count == 8 && s.t_min == 200 && s.t_max == 270);
    CHECK(s.t_sum / s.count == 235);
    CHECK(s.h_min == 500 && s.h_max == 640 && s.h_sum / s.count == 570);
    CHECK(stats_trend(&s, false) == 1 && stats_trend(&s, true) == 1);
    for (uint8_t i = 0; i < 8; i++)
        stats_add(&s, 270 - i * 10, 640 - i * 20);
    CHECK(stats_trend(&s, false) == -1 && stats_trend(&s, true) == -1);
    for (uint8_t i = 0; i < 8; i++)
        stats_add(&s, 250, 600);
    CHECK(stats_trend(&s, false) == 0 && stats_trend(&s, true) == 0);
    uint32_t count = s.count;
    stats_add(&s, 501, 600);
    CHECK(s.count == count);
    stats_reset(&s);
    for (uint32_t i = 0; i < STATS_LIMIT + 2; i++)
        stats_add(&s, 500, 1000);
    CHECK(s.count == STATS_LIMIT && s.t_sum == 500000000UL && s.h_sum == 1000000000UL);
    stats_reset(&s);
    CHECK(!s.count && !s.filled);
}
static void test_records(void) {
    settings_t s, n;
    settings_defaults(&s);
    uint8_t r[RECORD_BYTES], bad[RECORD_BYTES];
    uint32_t q;
    CHECK(crc16((const uint8_t *)"123456789", 9) == 0x29b1);
    record_pack(r, &s, 0xfffffffeUL);
    CHECK(record_unpack(r, &n, &q));
    CHECK(q == 0xfffffffeUL);
    CHECK(n.t_high == s.t_high && n.sound == s.sound);
    for (uint8_t i = 0; i < RECORD_BYTES; i++)
        for (uint8_t bit = 0; bit < 8; bit++) {
            memcpy(bad, r, sizeof r);
            bad[i] ^= 1 << bit;
            CHECK(!record_unpack(bad, &n, &q));
        }
    CHECK(sequence_newer(1, 0xffffffffUL));
    CHECK(!sequence_newer(0xffffffffUL, 1));
    CHECK(!sequence_newer(2, 2));
    /* Model each interruption point of actual commit sequence, previous slot stays valid. */
    settings_t next = s;
    next.t_high = 400;
    uint8_t old[RECORD_BYTES], fresh[RECORD_BYTES];
    record_pack(old, &s, 10);
    record_pack(fresh, &next, 11);
    for (uint8_t writes = 0; writes <= RECORD_BYTES + 1; writes++) {
        memcpy(bad, r, sizeof r);
        if (writes)
            bad[0] = 0;
        for (uint8_t i = 1; i < RECORD_BYTES && i < writes; i++)
            bad[i] = fresh[i];
        if (writes == RECORD_BYTES + 1)
            bad[0] = fresh[0];
        CHECK(record_unpack(old, &n, &q) && q == 10);
        if (writes && writes < RECORD_BYTES + 1)
            CHECK(!record_unpack(bad, &n, &q));
        if (writes == RECORD_BYTES + 1)
            CHECK(record_unpack(bad, &n, &q) && q == 11 && n.t_high == 400);
    }
}
static void test_rtc(void) {
    rtc_time_t t = {24, 2, 29, 23, 59, 59, 4}, n;
    uint8_t r[7];
    CHECK(rtc_valid(&t));
    rtc_encode(&t, r);
    CHECK(rtc_decode(r, &n) && !memcmp(&t, &n, sizeof t));
    t.year = 25;
    CHECK(!rtc_valid(&t));
    t.year = 24;
    t.day = 30;
    CHECK(!rtc_valid(&t));
    t.day = 29;
    t.month = 13;
    CHECK(!rtc_valid(&t));
    t.month = 2;
    t.weekday = 0;
    CHECK(!rtc_valid(&t));
    t.weekday = 4;
    rtc_encode(&t, r);
    r[2] = 0x72;
    CHECK(rtc_decode(r, &n) && n.hour == 12);
    r[2] = 0x52;
    CHECK(rtc_decode(r, &n) && n.hour == 0);
    r[2] = 0x61;
    CHECK(rtc_decode(r, &n) && n.hour == 13);
    r[2] = 0x40;
    CHECK(!rtc_decode(r, &n));
    r[2] = 0x19;
    r[0] = 0x6a;
    CHECK(!rtc_decode(r, &n));
    rtc_encode(&t, r);
    r[5] |= 0x80;
    CHECK(!rtc_decode(r, &n)); /* reject century >2099 */
}
static void test_dht_time(void) {
    uint8_t r[5] = {60, 5, 25, 2, 92};
    uint16_t t, h;
    CHECK(dht_decode(r, &t, &h) && t == 252 && h == 605);
    r[4]++;
    CHECK(!dht_decode(r, &t, &h));
    r[0] = 101;
    r[4] = 133;
    CHECK(!dht_decode(r, &t, &h));
    uint8_t max[5] = {100, 0, 50, 0, 150};
    CHECK(dht_decode(max, &t, &h) && t == 500 && h == 1000);
    CHECK(due(3, 0xfffffffeUL));
    CHECK(!due(0xfffffffeUL, 3));
    CHECK(due(4, 4));
}
int main(void) {
    test_settings();
    test_alarms();
    test_stats();
    test_records();
    test_rtc();
    test_dht_time();
    printf("PASS: %u assertions in 6 groups (settings, alarm, stats, EEPROM, RTC, DHT/time)\n",
           checks);
    return 0;
}
