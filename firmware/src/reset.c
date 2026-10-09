#include "hal.h"
#include <avr/io.h>
#include <avr/wdt.h>
uint8_t reset_cause __attribute__((section(".noinit")));
void early_reset(void) __attribute__((naked, section(".init3"), used));
void early_reset(void) {
    reset_cause = MCUCSR;
    MCUCSR = 0;
    wdt_disable();
}
