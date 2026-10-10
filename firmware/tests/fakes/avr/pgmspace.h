/* Host UI tests only. Firmware uses the real AVR-Libc pgmspace.h. */
#ifndef TEST_PGMSPACE_H
#define TEST_PGMSPACE_H
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#define PROGMEM
#define PSTR(s) (s)
typedef const char *PGM_P;
#define strcpy_P strcpy
static inline int snprintf_P(char *buffer, size_t size, PGM_P format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, size, format, args);
    va_end(args);
    return result;
}
#endif
