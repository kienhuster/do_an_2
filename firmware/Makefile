MCU := atmega16
PROFILE ?= base
AVR_PREFIX ?= avr-
CC := $(AVR_PREFIX)gcc
OBJCOPY := $(AVR_PREFIX)objcopy
SIZE := $(AVR_PREFIX)size
OUT := build/$(PROFILE)
SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,$(OUT)/%.o,$(SRC))
DEFS_base :=
DEFS_full := -DENABLE_BUZZER=1 -DENABLE_RTC=1 -DENABLE_BH1750=1
DEFS_rtc := -DENABLE_RTC=1
DEFS_light := -DENABLE_BH1750=1
DEFS_passive := $(DEFS_full) -DBUZZER_ACTIVE=0
DEFS_diagnostic := -DENABLE_LCD=0 -DENABLE_BUZZER=0 -DENABLE_RTC=0 -DENABLE_BH1750=0 -DDIAG_LED7=1
DEFS_minimal := -DENABLE_LCD=0 -DENABLE_UART=0 -DENABLE_EEPROM=0 -DENABLE_ADC=0 -DENABLE_DIAGNOSTIC=0
PROFILES := base full rtc light passive diagnostic minimal
ifeq ($(filter $(PROFILE),$(PROFILES)),)
$(error Unknown PROFILE '$(PROFILE)')
endif
CFLAGS := -mmcu=$(MCU) -DF_CPU=8000000UL -std=gnu99 -Os -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections -fstack-usage -Iinclude $(DEFS_$(PROFILE)) $(EXTRA_DEFS)
LDFLAGS := -mmcu=$(MCU) -Wl,--gc-sections,-Map,$(OUT)/monitor.map
.PHONY: all matrix test simulate size clean FORCE
all: $(OUT)/monitor.hex $(OUT)/monitor.eep size
$(OUT)/reset.o: CFLAGS := $(filter-out -fstack-usage,$(CFLAGS))
$(OUT)/.flags: FORCE
	@python3 tools/build_stamp.py $@ "$(CC)" "$(CFLAGS)" "$(LDFLAGS)"
$(OUT)/%.o: src/%.c $(wildcard include/*.h) $(OUT)/.flags
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
$(OUT)/monitor.elf: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@
$(OUT)/monitor.hex: $(OUT)/monitor.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@
$(OUT)/monitor.eep: $(OUT)/monitor.elf
	$(OBJCOPY) -O ihex -j .eeprom --change-section-lma .eeprom=0 $< $@
size: $(OUT)/monitor.elf
	python3 tools/check_size.py $(SIZE) $<
matrix:
	@set -e; for profile in $(PROFILES); do $(MAKE) PROFILE=$$profile all; done
test:
	@mkdir -p build/tests
	cc -std=c99 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Iinclude tests/test_core.c src/core.c -o build/tests/test_core
	./build/tests/test_core
simulate: all
	@mkdir -p build/tests
	cc -std=c99 -g -Wall -Wextra -Werror $(SIM_CFLAGS) tests/sim_smoke.c -o build/tests/sim_smoke $(SIM_LDFLAGS) -lsimavr
	./build/tests/sim_smoke $(OUT)/monitor.elf
clean:
	rm -rf build
-include $(OBJ:.o=.d)
