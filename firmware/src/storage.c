#include "hal.h"
#include <avr/eeprom.h>
#include <string.h>
#if ENABLE_EEPROM
static uint8_t EEMEM slots[2][RECORD_BYTES];
static uint8_t selected = 1;
static uint32_t sequence;
#endif
bool nv_load(settings_t *s) {
#if ENABLE_EEPROM
    uint8_t b[RECORD_BYTES];
    settings_t n;
    uint32_t seq;
    bool found = false;
    for (uint8_t i = 0; i < 2; i++) {
        eeprom_read_block(b, slots[i], sizeof(b));
        if (record_unpack(b, &n, &seq) && (!found || sequence_newer(seq, sequence))) {
            *s = n;
            sequence = seq;
            selected = i;
            found = true;
        }
    }
    return found;
#else
    (void)s;
    return false;
#endif
}
bool nv_save(const settings_t *s) {
#if ENABLE_EEPROM
    if (!settings_valid(s))
        return false;
    uint8_t b[RECORD_BYTES], verify[RECORD_BYTES];
    uint8_t target = selected ^ 1;
    record_pack(b, s, sequence + 1);
    /* Invalidate first, then payload+CRC, commit marker last. Old slot retained. */
    eeprom_update_byte(&slots[target][0], 0);
    for (uint8_t i = 1; i < RECORD_BYTES; i++)
        eeprom_update_byte(&slots[target][i], b[i]);
    eeprom_update_byte(&slots[target][0], b[0]);
    eeprom_read_block(verify, slots[target], sizeof(verify));
    settings_t n;
    uint32_t seq;
    if (memcmp(verify, b, sizeof b) || !record_unpack(verify, &n, &seq) || seq != sequence + 1)
        return false;
    selected = target;
    sequence = seq;
    return true;
#else
    (void)s;
    return false;
#endif
}
