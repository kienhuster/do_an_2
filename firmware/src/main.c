#include "app.h"
#include <avr/io.h>
#include <avr/wdt.h>
app_t app;
void app_ack(void) {
    alarm_ack(&app.alarm);
    app.fault_ack = app.errors;
}
bool app_save(void) {
#if ENABLE_EEPROM
    if (nv_save(&app.settings)) {
        app.errors &= ~ERR_EEPROM;
        return true;
    }
    app.errors |= ERR_EEPROM;
#endif
    return false;
}
bool app_load(void) {
#if ENABLE_EEPROM
    if (nv_load(&app.settings)) {
        app.errors &= ~ERR_EEPROM;
        ui_cancel();
        return true;
    }
    app.errors |= ERR_EEPROM;
#endif
    return false;
}
int main(void) {
    board_init();
    uart_init();
    lcd_init();
    dht_init();
    i2c_init();
    settings_defaults(&app.settings);
#if ENABLE_EEPROM
    if (!nv_load(&app.settings))
        app.errors |= ERR_EEPROM;
#endif
    if (reset_cause & _BV(WDRF))
        app.errors |= ERR_WATCHDOG;
    stats_reset(&app.stats);
    app.csv = ENABLE_UART;
    app.diagnostic = DIAG_LED7;
    if (DIAG_LED7)
        app.page = 8;
    uint32_t now = clock_ms(), prev = now, subsecond = 0;
    uint32_t next_dht = now + 2000, next_ui = now, next_adc = now,
             next_optional = now;
#if ENABLE_UART
    uint32_t next_csv = now + 2000;
#endif
#if ENABLE_BH1750
    bool light_ready = light_start();
    uint32_t light_next = clock_ms() + (light_ready ? 200 : 5000);
    if (!light_ready)
        app.errors |= ERR_LIGHT;
#endif
#if ENABLE_DHT11
    bool dht_pending = false;
    uint32_t dht_release = 0;
#else
    (void)next_dht;
#endif
    wdt_enable(WDTO_1S);
#if ENABLE_UART
    uart_text_P(PSTR("# ATmega16 monitor C 8MHz; HELP for commands\r\n"));
    csv_header();
#endif
    for (;;) {
        now = clock_ms();
        subsecond += now - prev;
        prev = now;
        while (subsecond >= 1000) {
            subsecond -= 1000;
            app.uptime++;
        }
#if ENABLE_UART
        command_poll();
#endif
        ui_keys(keys_poll(now));
#if ENABLE_DHT11
        if (!dht_pending && due(now, next_dht)) {
            dht_start();
            dht_pending = true;
            dht_release = now + 20;
            next_dht = now + DHT_PERIOD_MS;
        }
        if (dht_pending && due(now, dht_release)) {
            dht_pending = false;
            app.sample_valid = dht_finish(&app.temperature, &app.humidity);
            if (app.sample_valid) {
                app.dht_failures = 0;
                app.errors &= ~ERR_DHT;
                stats_add(&app.stats, app.temperature, app.humidity);
                alarm_update(&app.alarm, &app.settings, app.temperature, app.humidity);
            } else {
                if (app.dht_failures < 255)
                    app.dht_failures++;
                if (app.dht_failures >= 3)
                    app.errors |= ERR_DHT;
            }
        }
#endif
        if (due(now, next_adc)) {
            next_adc = now + 500;
#if ENABLE_ADC
            app.adc_valid = adc_read(&app.adc);
            if (app.adc_valid)
                app.errors &= ~ERR_ADC;
            else
                app.errors |= ERR_ADC;
#endif
        }
        if (due(now, next_optional)) {
            next_optional = now + 1000;
#if ENABLE_RTC
            app.rtc_valid = rtc_read(&app.rtc);
            if (app.rtc_valid)
                app.errors &= ~ERR_RTC;
            else
                app.errors |= ERR_RTC;
#endif
        }
#if ENABLE_BH1750
        if (due(now, light_next)) {
            if (!light_ready) {
                light_ready = light_start();
                light_next = now + (light_ready ? 200 : 5000);
            } else {
                app.light_valid = light_read(&app.lux);
                light_ready = app.light_valid;
                light_next = now + (light_ready ? 2000 : 5000);
            }
            if (app.light_valid)
                app.errors &= ~ERR_LIGHT;
            else
                app.errors |= ERR_LIGHT;
        }
#endif
#if ENABLE_UART
        if (uart_errors())
            app.errors |= ERR_UART;
#endif
        app.fault_ack &= app.errors;
        bool ring = (app.alarm.active & ~app.alarm.ack) ||
                    (app.errors & ~app.fault_ack &
                     (ERR_DHT | ERR_RTC | ERR_LIGHT | ERR_ADC | ERR_WATCHDOG));
        buzzer_set(app.settings.sound && ring && (now % 1000 < 200));
        diagnostic_tick(now, app.diagnostic);
        if (due(now, next_ui)) {
            next_ui = now + 250;
            ui_render();
        }
#if ENABLE_UART
        if (due(now, next_csv)) {
            next_csv = now + (uint32_t)app.settings.csv_seconds * 1000;
            if (app.csv)
                csv_send();
        }
#endif
        wdt_reset();
    }
}
