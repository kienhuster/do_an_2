#ifndef HAL_H
#define HAL_H
#include "core.h"
#include <avr/pgmspace.h>
enum { KEY_UP = 1, KEY_DOWN = 2, KEY_SET = 4, KEY_ACK = 8 };
extern uint8_t reset_cause;
void board_init(void);
uint32_t clock_ms(void);
uint8_t keys_poll(uint32_t now);
uint8_t keys_raw(void);
void buzzer_set(bool on);
void diagnostic_tick(uint32_t now, bool enabled);
void lcd_init(void);
void lcd_line(uint8_t row, const char *text);
bool adc_read(uint16_t *value);
void uart_init(void);
void uart_rx_poll(void);
bool uart_get(char *c);
bool uart_put(char c);
void uart_text_P(PGM_P text);
void uart_uint(uint32_t n);
void uart_fixed(uint16_t n);
uint8_t uart_errors(void);
void uart_clear_errors(void);
bool uart_take_rx_fault(void);
void dht_init(void);
void dht_start(void);
bool dht_finish(uint16_t *t10, uint16_t *h10);
void i2c_init(void);
bool i2c_write(uint8_t addr, const uint8_t *data, uint8_t size);
bool i2c_read(uint8_t addr, uint8_t *data, uint8_t size);
bool i2c_regs(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t size);
bool rtc_read(rtc_time_t *t);
bool rtc_set(const rtc_time_t *t);
bool light_start(void);
bool light_read(uint32_t *lux);
bool nv_load(settings_t *s);
bool nv_save(const settings_t *s);
#endif
