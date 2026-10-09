#include "app.h"
#include <string.h>
#include <stdio.h>
#define SAY(s) uart_text_P(PSTR(s))
static void comma(void) {
    uart_put(',');
}
static void signed_trend(int8_t v) {
    if (v < 0)
        uart_put('-');
    uart_uint(v < 0 ? -v : v);
}
static void two(uint8_t n) {
    uart_put('0' + n / 10);
    uart_put('0' + n % 10);
}
void csv_header(void) {
    SAY("uptime_s,temp_c,humidity_pct,t_trend,h_trend,alarm,ack,errors,adc_raw,lux,rtc\r\n");
}
void csv_send(void) {
    uart_uint(app.uptime);
    comma();
    if (app.sample_valid)
        uart_fixed(app.temperature);
    comma();
    if (app.sample_valid)
        uart_fixed(app.humidity);
    comma();
    if (app.sample_valid && app.stats.filled == TREND_SAMPLES)
        signed_trend(stats_trend(&app.stats, false));
    comma();
    if (app.sample_valid && app.stats.filled == TREND_SAMPLES)
        signed_trend(stats_trend(&app.stats, true));
    comma();
    uart_uint(app.alarm.active);
    comma();
    uart_uint(app.alarm.ack);
    comma();
    uart_uint(app.errors);
    comma();
    if (app.adc_valid)
        uart_uint(app.adc);
    comma();
    if (app.light_valid)
        uart_uint(app.lux);
    comma();
    if (app.rtc_valid) {
        SAY("20");
        two(app.rtc.year);
        uart_put('-');
        two(app.rtc.month);
        uart_put('-');
        two(app.rtc.day);
        uart_put('T');
        two(app.rtc.hour);
        uart_put(':');
        two(app.rtc.minute);
        uart_put(':');
        two(app.rtc.second);
    }
    SAY("\r\n");
}
#if ENABLE_UART
static void config_send(void) {
    SAY("# TLOW=");
    uart_uint(app.settings.t_low);
    SAY(" THIGH=");
    uart_uint(app.settings.t_high);
    SAY(" HLOW=");
    uart_uint(app.settings.h_low);
    SAY(" HHIGH=");
    uart_uint(app.settings.h_high);
    SAY("\r\n# THYST=");
    uart_uint(app.settings.t_hyst);
    SAY(" HHYST=");
    uart_uint(app.settings.h_hyst);
    SAY(" CSVSEC=");
    uart_uint(app.settings.csv_seconds);
    SAY(" SOUND=");
    uart_uint(app.settings.sound);
    SAY("\r\n");
}
static void stats_send(void) {
    SAY("# count=");
    uart_uint(app.stats.count);
    SAY("\r\n");
    if (!app.stats.count) {
        SAY("# no valid samples\r\n");
        return;
    }
    SAY("# T min,max,avg=");
    uart_fixed(app.stats.t_min);
    comma();
    uart_fixed(app.stats.t_max);
    comma();
    uart_fixed(app.stats.t_sum / app.stats.count);
    SAY("\r\n# H min,max,avg=");
    uart_fixed(app.stats.h_min);
    comma();
    uart_fixed(app.stats.h_max);
    comma();
    uart_fixed(app.stats.h_sum / app.stats.count);
    SAY("\r\n");
}
#if ENABLE_RTC
static bool set_time(char *p) {
    /* RTC YY-MM-DD HH:MM:SS weekday (1..7), exactly 21 characters. */
    if (strlen(p) != 21 || p[2] != '-' || p[5] != '-' || p[8] != ' ' || p[11] != ':' ||
        p[14] != ':' || p[17] != ' ' || p[18] != 'W' || p[19] != '=')
        return false;
    uint8_t values[6];
    const uint8_t offsets[] = {0, 3, 6, 9, 12, 15};
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t j = offsets[i];
        if (p[j] < '0' || p[j] > '9' || p[j + 1] < '0' || p[j + 1] > '9')
            return false;
        values[i] = (p[j] - '0') * 10 + p[j + 1] - '0';
    }
    if (p[20] < '1' || p[20] > '7')
        return false;
    rtc_time_t t = {values[0], values[1], values[2], values[3], values[4], values[5], p[20] - '0'};
    if (!rtc_set(&t))
        return false;
    app.rtc_valid = rtc_read(&app.rtc);
    if (app.rtc_valid)
        app.errors &= ~ERR_RTC;
    else
        app.errors |= ERR_RTC;
    return app.rtc_valid;
}
#endif
static bool execute(char *line) {
    if (!strcmp_P(line, PSTR("HELP"))) {
        SAY("# STATUS CONFIG STATS ACK SAVE LOAD DEFAULTS RESETSTATS\r\n");
        SAY("# CSV ON|OFF; SET TLOW|THIGH|HLOW|HHIGH|THYST|HHYST|CSVSEC|SOUND n\r\n");
        SAY("# PAGE 0..8; DIAG ON|OFF; CLEARERR\r\n");
        SAY("# RTC YY-MM-DD HH:MM:SS W=1..7 (2000..2099)\r\n");
        return true;
    }
    if (!strcmp_P(line, PSTR("STATUS"))) {
        csv_send();
        return true;
    }
    if (!strcmp_P(line, PSTR("CONFIG"))) {
        config_send();
        return true;
    }
    if (!strcmp_P(line, PSTR("STATS"))) {
        stats_send();
        return true;
    }
    if (!strcmp_P(line, PSTR("ACK"))) {
        app_ack();
        return true;
    }
    if (!strcmp_P(line, PSTR("SAVE")))
        return app_save();
    if (!strcmp_P(line, PSTR("LOAD")))
        return app_load();
    if (!strcmp_P(line, PSTR("DEFAULTS"))) {
        settings_defaults(&app.settings);
        ui_cancel();
        return true;
    }
    if (!strcmp_P(line, PSTR("RESETSTATS"))) {
        stats_reset(&app.stats);
        return true;
    }
    if (!strcmp_P(line, PSTR("CLEARERR"))) {
        uart_clear_errors();
        app.errors &= ~(ERR_UART | ERR_WATCHDOG);
        return true;
    }
    if (!strcmp_P(line, PSTR("CSV ON"))) {
        app.csv = true;
        csv_header();
        return true;
    }
    if (!strcmp_P(line, PSTR("CSV OFF"))) {
        app.csv = false;
        return true;
    }
    if (!strncmp_P(line, PSTR("SET "), 4)) {
        char *p = strchr(line + 4, ' ');
        int16_t n;
        if (!p)
            return false;
        *p++ = 0;
        if (!parse_i16(p, &n) || !settings_set(&app.settings, line + 4, n))
            return false;
        ui_cancel();
        return true;
    }
    if (!strncmp_P(line, PSTR("PAGE "), 5)) {
        int16_t n;
        if (!parse_i16(line + 5, &n) || n < 0 || n > 8)
            return false;
        app.page = n;
        ui_cancel();
        return true;
    }
#if ENABLE_DIAGNOSTIC
    if (!strcmp_P(line, PSTR("DIAG"))) {
        SAY("# KEY=");
        uart_uint(keys_raw());
        SAY(" RESET=");
        uart_uint(reset_cause);
        SAY(" LEDS=");
        uart_uint(app.diagnostic);
        SAY("\r\n");
        return true;
    }
    if (!strcmp_P(line, PSTR("DIAG ON"))) {
        app.diagnostic = true;
        app.page = 8;
        return true;
    }
    if (!strcmp_P(line, PSTR("DIAG OFF"))) {
        app.diagnostic = false;
        return true;
    }
#endif
#if ENABLE_RTC
    if (!strncmp_P(line, PSTR("RTC "), 4))
        return set_time(line + 4);
#endif
    return false;
}
#endif
void command_poll(void) {
#if ENABLE_UART
    static char line[48];
    static uint8_t used;
    static bool discard;
    if (uart_take_rx_fault())
        discard = true;
    char c;
    while (uart_get(&c)) {
        if (uart_take_rx_fault())
            discard = true;
        if (c == '\r' || c == '\n') {
            if (discard)
                SAY("# ERR line discarded\r\n");
            else if (used) {
                line[used] = 0;
                if (execute(line))
                    SAY("# OK\r\n");
                else
                    SAY("# ERR command/value/module\r\n");
            }
            used = 0;
            discard = false;
        } else if (c == '\b' || c == 127) {
            if (used && !discard)
                used--;
        } else if (c >= 32 && c <= 126) {
            if (used < sizeof(line) - 1)
                line[used++] = c;
            else
                discard = true;
        } else
            discard = true;
    }
#endif
}
