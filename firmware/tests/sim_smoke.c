/* Protocol models exist ONLY in this test runner, never in firmware drivers. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sim_avr.h>
#include <sim_elf.h>
#include <avr_uart.h>
#include <avr_ioport.h>
#include <avr_adc.h>
#include <avr_twi.h>
#include <avr_watchdog.h>
#include <sim_cycle_timers.h>
typedef struct {
    avr_t *avr;
    avr_irq_t *dht, *uart, *twi;
    char output[65536];
    size_t used;
    uint8_t lcd[128], cursor, nibble;
    bool four, pair, enable;
    bool sensor, corrupt, was_output, optional;
    uint8_t temperature, humidity, frame[5], phase;
    avr_cycle_count_t low_start;
    uint8_t selected, index, reg, rtc[32];
    unsigned rtc_reads, light_reads, light_commands, buzzer_edges;
    bool stream_on_dht;
    uint8_t stream_index;
    unsigned overlapping_bytes;
} model_t;
static unsigned checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        checks++;                                                                                  \
        if (!(x)) {                                                                                \
            fprintf(stderr, "FAIL %s:%d %s\nUART:\n%s\n", __FILE__, __LINE__, #x, m.output);       \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static void output(struct avr_irq_t *irq, uint32_t v, void *p) {
    (void)irq;
    model_t *m = p;
    if (m->used + 1 < sizeof m->output) {
        m->output[m->used++] = (char)v;
        m->output[m->used] = 0;
    }
}
static void run_ms(model_t *m, uint32_t ms) {
    avr_cycle_count_t remaining = (avr_cycle_count_t)ms * 8000;
    while (remaining) {
        avr_cycle_count_t before = m->avr->cycle;
        int state = avr_run(m->avr);
        if (state == cpu_Crashed || state == cpu_Done) {
            fprintf(stderr, "AVR stopped\n");
            exit(1);
        }
        avr_cycle_count_t step = m->avr->cycle >= before ? m->avr->cycle - before : 1;
        remaining = step < remaining ? remaining - step : 0;
    }
}
static void command(model_t *m, const char *s) {
    for (; *s; s++) {
        avr_raise_irq(m->uart, (uint8_t)*s);
        run_ms(m, 2);
    }
    avr_raise_irq(m->uart, '\n');
    run_ms(m, 400);
}
static void clear(model_t *m) {
    m->used = 0;
    m->output[0] = 0;
}
static void button(model_t *m, uint8_t pin, uint32_t hold) {
    avr_irq_t *key = avr_io_getirq(m->avr, AVR_IOCTL_IOPORT_GETIRQ('B'), pin);
    avr_raise_irq(key, 0);
    run_ms(m, hold);
    avr_raise_irq(key, 1);
    run_ms(m, 300);
}
static avr_cycle_count_t uart_stream(avr_t *avr, avr_cycle_count_t when, void *p) {
    model_t *m = p;
    const char *text = "CONFIG\n";
    if (!text[m->stream_index])
        return 0;
    if (!avr->sreg[S_I])
        m->overlapping_bytes++;
    avr_raise_irq(m->uart, (uint8_t)text[m->stream_index++]);
    return when + 1100 * 8;
}
static avr_cycle_count_t pulse(avr_t *avr, avr_cycle_count_t when, void *p) {
    (void)avr;
    model_t *m = p;
    uint8_t phase = m->phase++;
    unsigned duration;
    bool high;
    if (phase == 0) {
        high = false;
        duration = 80;
    } else if (phase == 1) {
        high = true;
        duration = 80;
    } else if (phase == 82) {
        high = false;
        duration = 50;
    } else if (phase == 83) {
        avr_raise_irq(m->dht, 1);
        return 0;
    } else if (!(phase & 1)) {
        high = false;
        duration = 50;
    } else {
        unsigned bit = (phase - 3) / 2;
        high = true;
        duration = (m->frame[bit / 8] & (0x80 >> (bit % 8))) ? 70 : 27;
    }
    avr_raise_irq(m->dht, high);
    return when + duration * 8;
}
static void direction(struct avr_irq_t *irq, uint32_t v, void *p) {
    (void)irq;
    model_t *m = p;
    bool out = (v & 2) != 0;
    if (out && !m->was_output)
        m->low_start = m->avr->cycle;
    if (!out && m->was_output && m->sensor && m->avr->cycle - m->low_start >= 18000 * 8) {
        m->frame[0] = m->humidity;
        m->frame[1] = 0;
        m->frame[2] = m->temperature;
        m->frame[3] = 0;
        m->frame[4] = m->humidity + m->temperature + (m->corrupt ? 1 : 0);
        m->phase = 0;
        avr_cycle_timer_register_usec(m->avr, 20, pulse, m);
        if (m->stream_on_dht) {
            m->stream_on_dht = false;
            m->stream_index = 0;
            avr_cycle_timer_register_usec(m->avr, 100, uart_stream, m);
        }
    }
    m->was_output = out;
}
static void lcd_enable(struct avr_irq_t *irq, uint32_t value, void *p) {
    (void)irq;
    model_t *m = p;
    bool high = value & 1;
    if (m->enable && !high) {
        avr_ioport_state_t c, d;
        avr_ioctl(m->avr, AVR_IOCTL_IOPORT_GETSTATE('C'), &c);
        avr_ioctl(m->avr, AVR_IOCTL_IOPORT_GETSTATE('D'), &d);
        uint8_t nib = c.port >> 4;
        if (!m->four) {
            if (nib == 2) {
                m->four = true;
                m->pair = false;
            }
        } else if (!m->pair) {
            m->nibble = nib;
            m->pair = true;
        } else {
            uint8_t byte = (m->nibble << 4) | nib;
            m->pair = false;
            if (d.port & 0x40) {
                m->lcd[m->cursor & 127] = byte;
                m->cursor++;
            } else if (byte & 0x80)
                m->cursor = byte & 0x7f;
            else if (byte == 1) {
                memset(m->lcd, ' ', 128);
                m->cursor = 0;
            }
        }
    }
    m->enable = high;
}
static void buzzer(struct avr_irq_t *irq, uint32_t value, void *p) {
    (void)irq;
    model_t *m = p;
    if (value & 1)
        m->buzzer_edges++;
}
static void twi_model(struct avr_irq_t *irq, uint32_t v, void *p) {
    (void)irq;
    model_t *m = p;
    avr_twi_msg_irq_t msg;
    msg.u.v = v;
    uint8_t event = msg.u.twi.msg, address = msg.u.twi.addr;
    if (getenv("TWI_TRACE"))
        fprintf(stderr, "TWI event=%x addr=%x data=%x\n", event, address, msg.u.twi.data);
    if (event & TWI_COND_STOP)
        m->selected = 0;
    if (event & TWI_COND_START) {
        m->selected = 0;
        m->index = 0;
        if (m->optional && (address >> 1 == 0x68 || address >> 1 == 0x23)) {
            m->selected = address;
            avr_raise_irq(m->twi, avr_twi_irq_msg(TWI_COND_ACK, address, 1));
        }
    }
    if (!m->selected)
        return;
    if (event & TWI_COND_WRITE) {
        uint8_t data = msg.u.twi.data;
        if (m->selected >> 1 == 0x68) {
            if (!m->index)
                m->reg = data;
            else
                m->rtc[m->reg++ & 31] = data;
        } else if (data == 1 || data == 7 || data == 0x10)
            m->light_commands++;
        m->index++;
        avr_raise_irq(m->twi, avr_twi_irq_msg(TWI_COND_ACK, m->selected, 1));
    }
    if (event & TWI_COND_READ) {
        uint8_t data;
        if (m->selected >> 1 == 0x68) {
            data = m->rtc[m->reg++ & 31];
            m->rtc_reads++;
        } else {
            data = m->index ? 0xb0 : 0x04;
            m->light_reads++;
        } /* raw 1200 => 1000 lux */
        m->index++;
        avr_raise_irq(m->twi, avr_twi_irq_msg(TWI_COND_READ, m->selected, data));
    }
}
static void status_trace(struct avr_irq_t *irq, uint32_t v, void *p) {
    (void)irq;
    (void)p;
    if (getenv("TWI_TRACE"))
        fprintf(stderr, "STATUS %x\n", v);
}
static void adc_trigger(struct avr_irq_t *irq, uint32_t value, void *p) {
    (void)irq;
    (void)value;
    model_t *m = p;
    avr_raise_irq(avr_io_getirq(m->avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_ADC0), 2500);
}
static uint16_t symbol(elf_firmware_t *fw, const char *name) {
    for (uint32_t i = 0; i < fw->symbolcount; i++)
        if (!strcmp(fw->symbol[i]->symbol, name))
            return fw->symbol[i]->addr & 0xffff;
    fprintf(stderr, "Missing ELF symbol %s\n", name);
    exit(1);
}
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: sim_smoke monitor.elf\n");
        return 2;
    }
    elf_firmware_t fw = {0};
    if (elf_read_firmware(argv[1], &fw))
        return 2;
    model_t m = {0};
    m.avr = avr_make_mcu_by_name("atmega16");
    if (!m.avr)
        return 2;
    avr_init(m.avr);
    /* simavr 1.8: ATmega16 has reset-only watchdog, no interrupt vector.
     * Its pseudo-IRQ pool remains unset, crashing notification re-registration
     * on reset. Supply the missing pool metadata in the test fixture only. */
    for (avr_io_t *io = m.avr->io_port; io; io = io->next) {
        if (!strcmp(io->kind, "watchdog")) {
            avr_watchdog_t *wd = (avr_watchdog_t *)io;
            if (!wd->watchdog.vector)
                for (unsigned i = 0; i < AVR_INT_IRQ_COUNT; i++)
                    wd->watchdog.irq[i].pool = &m.avr->irq_pool;
        }
    }

    m.avr->frequency = 8000000;
    m.avr->vcc = m.avr->avcc = m.avr->aref = 5000;
    avr_load_firmware(m.avr, &fw);
    m.sensor = true;
    m.optional = true;
    m.temperature = 25;
    m.humidity = 60;
    uint8_t date[7] = {0x30, 0x20, 0x14, 4, 0x08, 0x10, 0x26};
    memcpy(m.rtc, date, 7);
    m.uart = avr_io_getirq(m.avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_INPUT);
    uint32_t flags = 0;
    avr_ioctl(m.avr, AVR_IOCTL_UART_SET_FLAGS('0'), &flags);
    avr_irq_register_notify(avr_io_getirq(m.avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUTPUT),
                            output, &m);
    m.dht = avr_io_getirq(m.avr, AVR_IOCTL_IOPORT_GETIRQ('A'), IOPORT_IRQ_PIN1);
    avr_ioport_external_t pull = {.name = 'A', .mask = 2, .value = 2};
    avr_ioctl(m.avr, AVR_IOCTL_IOPORT_SET_EXTERNAL('A'), &pull);
    avr_irq_register_notify(
        avr_io_getirq(m.avr, AVR_IOCTL_IOPORT_GETIRQ('A'), IOPORT_IRQ_DIRECTION_ALL), direction,
        &m);
    avr_irq_register_notify(avr_io_getirq(m.avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN7),
                            lcd_enable, &m);
    avr_irq_register_notify(avr_io_getirq(m.avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN2),
                            buzzer, &m);
    avr_irq_register_notify(avr_io_getirq(m.avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_OUT_TRIGGER),
                            adc_trigger, &m);
    m.twi = avr_io_getirq(m.avr, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_INPUT);
    avr_irq_register_notify(avr_io_getirq(m.avr, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_OUTPUT),
                            twi_model, &m);
    avr_irq_register_notify(avr_io_getirq(m.avr, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_STATUS),
                            status_trace, &m);
    run_ms(&m, 500);
    /* ATmega16 UCSRB is IO 0x0a, data-space 0x2a. RXEN/TXEN must stay zero. */
    if (!(m.avr->data[0x2a] & 0x18)) {
        CHECK(!m.used);
        CHECK(m.four && !memcmp(m.lcd, "DHT11 no data", 13));
        CHECK((m.avr->data[0x37] & 0xf0) == 0); /* DDRB: ISP pins undriven */
        CHECK(m.avr->data[0x54] & 0x80); /* JTD */
        run_ms(&m, 1800);
        CHECK(!memcmp(m.lcd, "T25.0C H60.0%", 13));
        button(&m, 0, 5); CHECK(!memcmp(m.lcd, "T25.0C", 6));
        button(&m, 0, 800); CHECK(m.lcd[0] == 'T' && m.lcd[1] == ' ');
        button(&m, 1, 50); CHECK(!memcmp(m.lcd, "T25.0C", 6));
        button(&m, 2, 50); CHECK(!memcmp(m.lcd, "TLOW=180", 8));
        button(&m, 0, 50); CHECK(!memcmp(m.lcd, "TLOW=190", 8));
        button(&m, 3, 50); CHECK(!memcmp(m.lcd, "T25.0C", 6));
        button(&m, 2, 50); CHECK(!memcmp(m.lcd, "TLOW=180", 8));
        button(&m, 2, 50); CHECK(!memcmp(m.lcd, "THIGH=350", 9));
        for (unsigned i = 0; i < 11; i++) button(&m, 1, 50);
        CHECK(!memcmp(m.lcd, "THIGH=240", 9));
        for (unsigned i = 0; i < 7; i++) button(&m, 2, 50);
        CHECK(!memcmp(m.lcd, "SET save & exit", 15));
        button(&m, 2, 50); run_ms(&m, 2200);
        CHECK(!memcmp(m.lcd + 0x40, "A:02 ACK:0 E:00", 15));
        button(&m, 3, 50); CHECK(!memcmp(m.lcd + 0x40, "A:02 ACK:2 E:00", 15));
        m.temperature = 23; run_ms(&m, 2200);
        CHECK(!memcmp(m.lcd + 0x40, "A:00 ACK:0 E:00", 15));
        m.temperature = 25; run_ms(&m, 2200);
        CHECK(!memcmp(m.lcd + 0x40, "A:02 ACK:0 E:00", 15));
        m.corrupt = true; run_ms(&m, 6500);
        CHECK(!memcmp(m.lcd, "DHT11 no data", 13));
        CHECK(!memcmp(m.lcd + 0x40, "A:02 ACK:0 E:01", 15));
        m.corrupt = false; run_ms(&m, 2200);
        CHECK(!memcmp(m.lcd, "T25.0C", 6));
        m.sensor = false; run_ms(&m, 6500);
        CHECK(!memcmp(m.lcd + 0x40, "A:02 ACK:0 E:01", 15));
        m.sensor = true; run_ms(&m, 2200);
        CHECK(!memcmp(m.lcd + 0x40, "A:02 ACK:0 E:00", 15));
        for (unsigned i = 0; i < 5; i++) button(&m, 0, 50);
        CHECK(!memcmp(m.lcd + 0x40, "511 ", 4));
        button(&m, 1, 50);
        uint32_t seconds = strtoul((char *)m.lcd + 0x40, NULL, 10);
        run_ms(&m, 5000);
        CHECK(strtoul((char *)m.lcd + 0x40, NULL, 10) == seconds + 5);
        /* UART faults injected at the peripheral must not enter the OFF application. */
        avr_raise_irq(m.uart, UART_INPUT_FE | '!'); run_ms(&m, 300);
        CHECK(!m.used && !(m.avr->data[0x2a] & 0x18));
        m.avr->flash[0x3ffc] = 0xff; m.avr->flash[0x3ffd] = 0xcf;
        m.avr->pc = 0x3ffc; run_ms(&m, 3500);
        CHECK(!memcmp(m.lcd + 0x40, "A:02 ACK:0 E:40", 15));
        button(&m, 2, 50); button(&m, 2, 50);
        CHECK(!memcmp(m.lcd, "THIGH=240", 9));
        CHECK(!m.used && !(m.avr->data[0x2a] & 0x18));
        printf("PASS: %u UART OFF simulator assertions: DHT/LCD, all four buttons, menu "
               "cancel/save, alarms/ACK/rearm, EEPROM across watchdog, ADC, uptime, ISP/JTAG\n", checks);
        avr_terminate(m.avr);
        return 0;
    }
    CHECK(strstr(m.output, "ATmega16 monitor C"));
    command(&m, "CSV OFF");
    CHECK(strstr(m.output, "# OK"));
    uint16_t end = symbol(&fw, "__bss_end") + 1;
    uint16_t sp = m.avr->data[0x5d] | ((uint16_t)m.avr->data[0x5e] << 8);
    for (uint16_t i = end; i + 32 < sp; i++)
        m.avr->data[i] = 0xa5;
    clear(&m);
    command(&m, "CONFIG");
    CHECK(strstr(m.output, "TLOW=180 THIGH=350"));
    run_ms(&m, 1800);
    clear(&m);
    command(&m, "STATUS");
    CHECK(strstr(m.output, ",25.0,60.0,"));
    CHECK(strstr(m.output, ",511,"));
    CHECK(m.four && !memcmp(m.lcd, "T25.0C H60.0%", 13));
    bool full = strstr(argv[1], "/full/") || strstr(argv[1], "/passive/") ||
                strstr(argv[1], "/legacy-full/") || strstr(argv[1], "/legacy-passive/");
    if (full) {
        CHECK(m.rtc_reads > 0 && m.light_reads >= 2 && m.light_commands >= 3);
        CHECK(strstr(m.output, ",1000,2026-10-08T14:20:30"));
    }
    clear(&m);
    command(&m, "SET THIGH 240");
    command(&m, "SAVE");
    command(&m, "DEFAULTS");
    command(&m, "LOAD");
    command(&m, "CONFIG");
    CHECK(strstr(m.output, "THIGH=240"));
    CHECK(!strstr(m.output, "# ERR"));
    run_ms(&m, 2100);
    clear(&m);
    command(&m, "ACK");
    command(&m, "STATUS");
    CHECK(strstr(m.output, ",2,2,0,"));
    if (full)
        CHECK(m.buzzer_edges > 0);
    m.temperature = 23;
    run_ms(&m, 2200);
    clear(&m);
    command(&m, "STATUS");
    CHECK(strstr(m.output, ",0,0,0,"));
    m.temperature = 25;
    run_ms(&m, 2200);
    clear(&m);
    command(&m, "STATUS");
    CHECK(strstr(m.output, ",2,0,0,"));
    m.corrupt = true;
    run_ms(&m, 6500);
    clear(&m);
    command(&m, "STATUS");
    CHECK(strstr(m.output, ",,,,,2,0,1,"));
    m.corrupt = false;
    run_ms(&m, 2200);
    clear(&m);
    command(&m, "STATUS");
    CHECK(strstr(m.output, ",25.0,60.0,"));
    clear(&m);
    m.stream_on_dht = true;
    run_ms(&m, 2500);
    CHECK(m.overlapping_bytes > 0 && strstr(m.output, "TLOW=180 THIGH=240"));
    CHECK(!strstr(m.output, "# ERR"));
    command(&m, "STATUS");
    CHECK(strstr(m.output, ",25.0,60.0,"));
    CHECK(strstr(m.output, ",2,0,0,"));
    m.sensor = false;
    run_ms(&m, 6500);
    clear(&m);
    command(&m, "STATUS");
    CHECK(strstr(m.output, ",,,,,2,0,1,"));
    m.sensor = true;
    run_ms(&m, 2200);
    clear(&m);
    command(&m, "STATUS");
    CHECK(strstr(m.output, ",25.0,60.0,"));
    if (full) {
        m.optional = false;
        run_ms(&m, 6500);
        clear(&m);
        command(&m, "STATUS");
        CHECK(strstr(m.output, ",2,0,6,"));
        m.optional = true;
        run_ms(&m, 6000);
        clear(&m);
        command(&m, "STATUS");
        CHECK(strstr(m.output, ",1000,2026-10-08"));
        CHECK(strstr(m.output, ",2,0,0,"));
        clear(&m);
        m.rtc[0x0e] = 0x80;
        m.rtc[0x0f] = 0x80;
        command(&m, "RTC 24-02-29 23:59:59 W=4");
        CHECK(!(m.rtc[0x0e] & 0x80) && !(m.rtc[0x0f] & 0x80));
        CHECK(!strstr(m.output, "# ERR"));
        CHECK(m.rtc[0] == 0x59 && m.rtc[4] == 0x29 && m.rtc[6] == 0x24);
        clear(&m);
        command(&m, "RTC 25-02-29 23:59:59 W=4");
        CHECK(strstr(m.output, "# ERR"));
    }
    clear(&m);
    command(&m, "SET THIGH 9999999999999999999999999999999999999999999999999999999");
    CHECK(strstr(m.output, "line discarded"));
    clear(&m);
    command(&m, "SET THIGH 100");
    CHECK(strstr(m.output, "# ERR"));
    command(&m, "CONFIG");
    CHECK(strstr(m.output, "THIGH=240"));
    clear(&m);
    avr_raise_irq(m.uart, UART_INPUT_FE | '!');
    run_ms(&m, 5);
    command(&m, "SET SOUND 0");
    CHECK(strstr(m.output, "line discarded"));
    command(&m, "CONFIG");
    CHECK(strstr(m.output, "SOUND=1"));
    command(&m, "CLEARERR");
    avr_irq_t *up = avr_io_getirq(m.avr, AVR_IOCTL_IOPORT_GETIRQ('B'), IOPORT_IRQ_PIN0);
    avr_raise_irq(up, 0);
    run_ms(&m, 5);
    avr_raise_irq(up, 1);
    run_ms(&m, 300);
    CHECK(!memcmp(m.lcd, "T25.0C", 6));
    avr_raise_irq(up, 0);
    run_ms(&m, 50);
    avr_raise_irq(up, 1);
    run_ms(&m, 300);
    CHECK(m.lcd[0] == 'T' && m.lcd[1] == ' ');
    clear(&m);
    command(&m, "HELP");
    CHECK(strstr(m.output, "RESETSTATS"));
    uint16_t first = end;
    while (first + 32 < sp && m.avr->data[first] == 0xa5)
        first++;
    CHECK(first > end + 128);
    printf("Measured stack watermark: >=%u bytes untouched above static data (tested paths only)\n",
           first - end);
    /* Inject an infinite loop in unused flash to test an actual watchdog reset. */
    clear(&m);
    m.avr->flash[0x3ffc] = 0xff;
    m.avr->flash[0x3ffd] = 0xcf;
    m.avr->pc = 0x3ffc;
    run_ms(&m, 1800);
    CHECK(strstr(m.output, "ATmega16 monitor C"));
    clear(&m);
    command(&m, "STATUS");
    CHECK(strstr(m.output, ",64,"));
    command(&m, "CONFIG");
    CHECK(strstr(m.output, "THIGH=240"));
    printf("PASS: %u simulator assertions: GPIO DHT/LCD/buttons, UART, ADC, EEPROM, alarms, "
           "recovery, watchdog%s\n",
           checks, full ? ", DS3231/BH1750/buzzer" : "");
    avr_terminate(m.avr);
    return 0;
}
