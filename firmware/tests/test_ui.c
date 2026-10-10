#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
app_t app;
uint8_t reset_cause;
static unsigned checks, saves;
static settings_t saved;
static char lines[2][17];
#define CHECK(x) do { checks++; if (!(x)) { \
    fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)
void lcd_line(uint8_t row, const char *text) {
    snprintf(lines[row], sizeof lines[row], "%s", text);
}
uint8_t keys_raw(void) { return 0; }
void app_ack(void) { alarm_ack(&app.alarm); app.fault_ack = app.errors; }
bool app_save(void) { saved = app.settings; saves++; return true; }
static void render(void) { ui_render(); }
int main(void) {
    settings_defaults(&app.settings);
    stats_reset(&app.stats);
    ui_keys(0); render();
    CHECK(app.page == 0 && saves == 0);
    ui_keys(KEY_DOWN); CHECK(app.page == 8);
    ui_keys(KEY_UP); CHECK(app.page == 0);
    ui_keys(KEY_SET); render(); CHECK(!strcmp(lines[0], "TLOW=180"));
    ui_keys(KEY_UP); render(); CHECK(!strcmp(lines[0], "TLOW=190"));
    CHECK(app.settings.t_low == 180 && saves == 0);
    ui_keys(KEY_ACK); render(); CHECK(app.settings.t_low == 180 && saves == 0);
    ui_keys(KEY_SET); ui_keys(KEY_DOWN); render();
    CHECK(!strcmp(lines[0], "TLOW=170"));
    ui_keys(KEY_SET); render(); CHECK(!strcmp(lines[0], "THIGH=350"));
    ui_keys(KEY_UP);
    for (unsigned i = 0; i < 7; i++) ui_keys(KEY_SET);
    render(); CHECK(!strcmp(lines[0], "SET save & exit"));
    ui_keys(KEY_SET);
    CHECK(saves == 1 && saved.t_low == 170 && saved.t_high == 360);
    CHECK(app.settings.t_high == 360);
    /* Cancel with simultaneous SET must not commit a pending edit. */
    ui_keys(KEY_SET); ui_keys(KEY_UP); ui_keys(KEY_ACK | KEY_SET);
    CHECK(saves == 1 && app.settings.t_low == 170);
    app.alarm.active = AL_T_HIGH | AL_H_LOW; app.errors = ERR_DHT;
    ui_keys(KEY_ACK);
    CHECK(app.alarm.ack == app.alarm.active && app.fault_ack == ERR_DHT);
    ui_keys(KEY_DOWN); ui_keys(KEY_SET); CHECK(app.diagnostic);
    ui_keys(KEY_SET); CHECK(!app.diagnostic);
    app.page = 0; ui_keys(KEY_SET);
    for (unsigned i = 0; i < 100; i++) ui_keys(KEY_UP);
    render(); CHECK(!strcmp(lines[0], "TLOW=340"));
    ui_keys(KEY_ACK); CHECK(app.settings.t_low == 170);
    app.sample_valid = true; app.temperature = 250; app.humidity = 600;
    render(); CHECK(!strcmp(lines[0], "T25.0C H60.0%"));
    app.adc_valid = true; app.adc = 1023; app.page = 5;
    render(); CHECK(!strcmp(lines[1], "1023 5000mV @5V"));
    printf("PASS: %u UI assertions (four keys, pages, cancel, thresholds, save, ACK, ADC)\n", checks);
    return 0;
}
