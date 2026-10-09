#ifndef CONFIG_H
#define CONFIG_H
/* Override with -D or edit defaults. AVR-GCC GNU C only; clock fixed at 8 MHz. */
#ifndef F_CPU
#define F_CPU 8000000UL
#endif
#if F_CPU != 8000000UL
#error "This board timing configuration requires an 8 MHz clock"
#endif
#ifndef ENABLE_DHT11
#define ENABLE_DHT11 1
#endif
#ifndef ENABLE_LCD
#define ENABLE_LCD 1
#endif
#ifndef ENABLE_UART
#define ENABLE_UART 1
#endif
#ifndef ENABLE_EEPROM
#define ENABLE_EEPROM 1
#endif
#ifndef ENABLE_ADC
#define ENABLE_ADC 1
#endif
#ifndef ENABLE_BUZZER
#define ENABLE_BUZZER 0
#endif
#ifndef ENABLE_RTC
#define ENABLE_RTC 0
#endif
#ifndef ENABLE_BH1750
#define ENABLE_BH1750 0
#endif
#ifndef ENABLE_DIAGNOSTIC
#define ENABLE_DIAGNOSTIC 1
#endif
#ifndef DIAG_LED7
#define DIAG_LED7 0
#endif
#if DIAG_LED7 && (ENABLE_LCD || ENABLE_RTC || ENABLE_BH1750)
#error "LED7 owns PORTC: disable LCD and both I2C modules"
#endif
#ifndef BUZZER_ACTIVE
#define BUZZER_ACTIVE 1
#endif
#define BH1750_ADDRESS 0x23 /* ADDR low; use 0x5c if ADDR high */
#define UART_BAUD 9600UL
#define DHT_PERIOD_MS 2000UL
#define TREND_SAMPLES 8
#define STATS_LIMIT 1000000UL
#endif
