#include "app.h"
#include <stdio.h>
#include <string.h>
static uint8_t menu;
static settings_t edit;
void ui_cancel(void) {
    menu = 0;
}
static uint16_t menu_value(void) {
    switch (menu) {
    case 1:
        return edit.t_low;
    case 2:
        return edit.t_high;
    case 3:
        return edit.h_low;
    case 4:
        return edit.h_high;
    case 5:
        return edit.t_hyst;
    case 6:
        return edit.h_hyst;
    case 7:
        return edit.csv_seconds;
    case 8:
        return edit.sound;
    default:
        return 0;
    }
}
static void menu_adjust(int8_t direction) {
    settings_t n = edit;
    int16_t v = (int16_t)menu_value() + direction * ((menu <= 4) ? 10 : 1);
    if (v < 0)
        return;
    switch (menu) {
    case 1:
        n.t_low = v;
        break;
    case 2:
        n.t_high = v;
        break;
    case 3:
        n.h_low = v;
        break;
    case 4:
        n.h_high = v;
        break;
    case 5:
        n.t_hyst = v;
        break;
    case 6:
        n.h_hyst = v;
        break;
    case 7:
        if (v > 60)
            return;
        n.csv_seconds = v;
        break;
    case 8:
        if (v > 1)
            return;
        n.sound = v;
        break;
    default:
        return;
    }
    if (settings_valid(&n))
        edit = n;
}
void ui_keys(uint8_t keys) {
    if (keys & KEY_ACK) {
        app_ack();
        if (menu) {
            ui_cancel();
            return;
        }
    }
    if (menu) {
        if (keys & KEY_UP)
            menu_adjust(1);
        if (keys & KEY_DOWN)
            menu_adjust(-1);
        if (keys & KEY_SET) {
            if (menu == 9) {
                app.settings = edit;
                app_save();
                ui_cancel();
            } else
                menu++;
        }
        return;
    }
    if (keys & KEY_UP)
        app.page = (app.page + 1) % 9;
    if (keys & KEY_DOWN)
        app.page = (app.page + 8) % 9;
    if (keys & KEY_SET) {
        if (app.page == 8)
            app.diagnostic = !app.diagnostic;
        else {
            edit = app.settings;
            menu = 1;
        }
    }
}
void ui_render(void) {
#if ENABLE_LCD
    char b[17];
    if (menu) {
        if (menu == 9) {
            lcd_line(0, "SET save & exit");
            lcd_line(1, "ACK cancel");
            return;
        }
        const char *label;
        switch (menu) {
        case 1:
            label = PSTR("TLOW");
            break;
        case 2:
            label = PSTR("THIGH");
            break;
        case 3:
            label = PSTR("HLOW");
            break;
        case 4:
            label = PSTR("HHIGH");
            break;
        case 5:
            label = PSTR("THYST");
            break;
        case 6:
            label = PSTR("HHYST");
            break;
        case 7:
            label = PSTR("CSVSEC");
            break;
        default:
            label = PSTR("SOUND");
            break;
        }
        char name[8];
        strcpy_P(name, label);
        snprintf_P(b, sizeof b, PSTR("%s=%u"), name, menu_value());
        lcd_line(0, b);
        strcpy_P(b, PSTR("UP/DN SET> ACK X"));
        lcd_line(1, b);
        return;
    }
    switch (app.page) {
    case 0:
        if (app.sample_valid)
            snprintf_P(b, sizeof b, PSTR("T%u.%uC H%u.%u%%"), app.temperature / 10,
                       app.temperature % 10, app.humidity / 10, app.humidity % 10);
        else
            strcpy_P(b, ENABLE_DHT11 ? PSTR("DHT11 no data") : PSTR("DHT11 disabled"));
        lcd_line(0, b);
        snprintf_P(b, sizeof b, PSTR("A:%02X ACK:%X E:%02X"), app.alarm.active, app.alarm.ack,
                   app.errors);
        break;
    case 1:
    case 2: {
        bool h = app.page == 2;
        uint16_t lo = h ? app.stats.h_min : app.stats.t_min,
                 hi = h ? app.stats.h_max : app.stats.t_max;
        if (!app.stats.count) {
            strcpy_P(b, PSTR("Stats: no data"));
            lcd_line(0, b);
            strcpy_P(b, PSTR("Wait for DHT11"));
            break;
        }
        snprintf_P(b, sizeof b, PSTR("%c %u.%u-%u.%u"), h ? 'H' : 'T', lo / 10, lo % 10, hi / 10,
                   hi % 10);
        lcd_line(0, b);
        uint16_t avg = (h ? app.stats.h_sum : app.stats.t_sum) / app.stats.count;
        snprintf_P(b, sizeof b, PSTR("AVG%u.%u N%lu"), avg / 10, avg % 10,
                   (unsigned long)app.stats.count);
        break;
    }
    case 3:
        snprintf_P(b, sizeof b, PSTR("Trend window %u/8"), app.stats.filled);
        lcd_line(0, b);
        if (!app.sample_valid)
            strcpy_P(b, PSTR("Trend stale"));
        else {
            int8_t t = stats_trend(&app.stats, false), h = stats_trend(&app.stats, true);
            snprintf_P(b, sizeof b, PSTR("T:%c H:%c (+up)"),
                       t > 0   ? '+'
                       : t < 0 ? '-'
                               : '=',
                       h > 0   ? '+'
                       : h < 0 ? '-'
                               : '=');
        }
        break;
    case 4:
        snprintf_P(b, sizeof b, PSTR("UP %lud %02lu:%02lu"), (unsigned long)(app.uptime / 86400),
                   (unsigned long)(app.uptime / 3600 % 24), (unsigned long)(app.uptime / 60 % 60));
        lcd_line(0, b);
        snprintf_P(b, sizeof b, PSTR("%lus RESET:%02X"), (unsigned long)app.uptime, reset_cause);
        break;
    case 5:
        strcpy_P(b, PSTR("VR1 ADC0 / AVCC"));
        lcd_line(0, b);
        if (app.adc_valid)
            snprintf_P(b, sizeof b, PSTR("%u %lumV @5V"), app.adc,
                       (unsigned long)app.adc * 5000 / 1023);
        else
            strcpy_P(b, ENABLE_ADC ? PSTR("ADC error") : PSTR("ADC disabled"));
        break;
    case 6:
        if (app.rtc_valid) {
            snprintf_P(b, sizeof b, PSTR("20%02u-%02u-%02u"), app.rtc.year, app.rtc.month,
                       app.rtc.day);
            lcd_line(0, b);
            snprintf_P(b, sizeof b, PSTR("%02u:%02u:%02u W%u"), app.rtc.hour, app.rtc.minute,
                       app.rtc.second, app.rtc.weekday);
        } else {
            strcpy_P(b, PSTR("RTC DS3231"));
            lcd_line(0, b);
            strcpy_P(b, ENABLE_RTC ? PSTR("Missing / set UTC") : PSTR("RTC disabled"));
        }
        break;
    case 7:
        strcpy_P(b, PSTR("Light BH1750"));
        lcd_line(0, b);
        if (app.light_valid)
            snprintf_P(b, sizeof b, PSTR("%lu lux"), (unsigned long)app.lux);
        else
            strcpy_P(b, ENABLE_BH1750 ? PSTR("Light error") : PSTR("Light disabled"));
        break;
    default:
        snprintf_P(b, sizeof b, PSTR("DIAG:%u KEY:%X"), app.diagnostic, keys_raw());
        lcd_line(0, b);
        snprintf_P(b, sizeof b, PSTR("SET LED E:%02X"), app.errors);
        break;
    }
    lcd_line(1, b);
#endif
}
