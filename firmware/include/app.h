#ifndef APP_H
#define APP_H
#include "hal.h"
typedef struct {
    settings_t settings;
    stats_t stats;
    alarm_t alarm;
    uint32_t uptime, lux;
    uint16_t temperature, humidity, adc;
    rtc_time_t rtc;
    uint8_t errors, fault_ack, dht_failures, page;
    bool sample_valid, rtc_valid, light_valid, adc_valid, csv, diagnostic;
} app_t;
extern app_t app;
void app_ack(void);
bool app_save(void);
bool app_load(void);
void command_poll(void);
void csv_header(void);
void csv_send(void);
void ui_keys(uint8_t keys);
void ui_render(void);
void ui_cancel(void);
#endif
