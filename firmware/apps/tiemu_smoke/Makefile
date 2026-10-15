# SPDX-License-Identifier: GPL-2.0-or-later

ROOT       := $(abspath ../../..)
BUILD      ?= $(ROOT)/build/firmware/apps/tiemu_smoke
TIEMU_ROOT ?= $(ROOT)/third_party/tiemu
TIEMU_SRC  := $(TIEMU_ROOT)/tiemu/trunk/src
UAE        := $(TIEMU_SRC)/core/uae
RISCV_BIN  ?= /home/vash/tools/xpack-riscv-none-elf-gcc-15.2.0-1/bin
CROSS      := $(RISCV_BIN)/riscv-none-elf-
CC         := $(CROSS)gcc
SIZE       := $(CROSS)size

ARCH       := -march=rv32imac_zicsr_zifencei -mabi=ilp32
COMMON     := $(ARCH) -O2 -g -fcommon -ffunction-sections -fdata-sections
CPPFLAGS   := -DNO_GDB -DNO_SOUND -D__inline__=inline -DSTATFS_NO_ARGS=2 \
              -DSTATBUF_BAVAIL=f_bavail \
              -I$(ROOT)/src/port -I$(ROOT)/firmware/include \
              -I$(ROOT)/firmware/common \
              -I$(TIEMU_SRC) -I$(TIEMU_SRC)/core -I$(UAE) \
              -I$(TIEMU_SRC)/core/ti_hw -I$(TIEMU_SRC)/core/ti_sw \
              -I$(TIEMU_SRC)/core/dbg -I$(TIEMU_SRC)/misc
CFLAGS     := $(COMMON) -std=gnu11
ASFLAGS    := $(ARCH) -g
LDFLAGS    := $(ARCH) -nostdlib -static -T $(ROOT)/firmware/rt/link.ld \
              -Wl,--gc-sections -Wl,--orphan-handling=error \
              -Wl,--no-warn-rwx-segments -Wl,-Map,$(BUILD)/tiemu_smoke.map
LDLIBS     := -Wl,--start-group -lm -lc -lgcc -Wl,--end-group

CORE_SOURCES := \
    $(UAE)/cpuemu.c $(UAE)/cpudefs.c $(UAE)/cpustbl.c \
    $(UAE)/readcpu.c $(UAE)/fpp.c $(UAE)/missing.c $(UAE)/xmalloc.c \
    $(UAE)/newcpu.c \
    $(TIEMU_SRC)/core/ti_hw/flash.c \
    $(TIEMU_SRC)/core/ti_hw/gscales.c \
    $(TIEMU_SRC)/core/ti_hw/hw.c \
    $(TIEMU_SRC)/core/ti_hw/hwprot.c \
    $(TIEMU_SRC)/core/ti_hw/kbd.c \
    $(TIEMU_SRC)/core/ti_hw/m68k.c \
    $(TIEMU_SRC)/core/ti_hw/mem.c \
    $(TIEMU_SRC)/core/ti_hw/mem89.c \
    $(TIEMU_SRC)/core/ti_hw/mem89tm.c \
    $(TIEMU_SRC)/core/ti_hw/mem92.c \
    $(TIEMU_SRC)/core/ti_hw/mem92p.c \
    $(TIEMU_SRC)/core/ti_hw/memv2.c \
    $(TIEMU_SRC)/core/ti_hw/ports.c \
    $(TIEMU_SRC)/core/ti_hw/rtc_hw3.c \
    $(TIEMU_SRC)/core/ti_hw/tichars.c \
    $(TIEMU_SRC)/core/dbg/bkpts.c \
    $(ROOT)/src/port/tiemu_port.c
RT_SOURCES := $(ROOT)/firmware/rt/crt0.S $(ROOT)/firmware/rt/syscalls.c \
              $(ROOT)/firmware/common/uart.c
T89_KEY_SOURCE := $(ROOT)/firmware/common/t89_key.c
T89_LCD_SOURCES := $(ROOT)/firmware/common/t89_lcd.c \
                   $(ROOT)/firmware/common/t89_lcd_gray.c
APP_SOURCES := main.c flash.S $(T89_KEY_SOURCE) $(T89_LCD_SOURCES)

core_object = $(BUILD)/obj/core/$(notdir $(1)).o
rt_object   = $(BUILD)/obj/rt/$(notdir $(1)).o
app_object  = $(BUILD)/obj/app/$(notdir $(1)).o
CORE_OBJECTS := $(foreach source,$(CORE_SOURCES),$(call core_object,$(source)))
RT_OBJECTS   := $(foreach source,$(RT_SOURCES),$(call rt_object,$(source)))
APP_OBJECTS  := $(foreach source,$(APP_SOURCES),$(call app_object,$(source)))

all: $(BUILD)/tiemu_smoke.elf

$(BUILD)/tiemu_smoke.elf: $(RT_OBJECTS) $(APP_OBJECTS) $(CORE_OBJECTS) \
                          $(ROOT)/firmware/rt/link.ld
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $(RT_OBJECTS) $(APP_OBJECTS) $(CORE_OBJECTS) $(LDLIBS)
	$(SIZE) $@

define compile_c
$(1): $(2) $(MAKEFILE_LIST)
	@mkdir -p $$(dir $$@)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -MMD -MP -c -o $$@ $(2)
endef

define compile_s
$(1): $(2) $(MAKEFILE_LIST)
	@mkdir -p $$(dir $$@)
	$$(CC) $$(CPPFLAGS) $$(ASFLAGS) -MMD -MP -c -o $$@ $(2)
endef

$(foreach source,$(CORE_SOURCES),$(eval $(call compile_c,$(call core_object,$(source)),$(source))))
$(foreach source,$(filter %.c,$(RT_SOURCES)),$(eval $(call compile_c,$(call rt_object,$(source)),$(source))))
$(foreach source,$(filter %.S,$(RT_SOURCES)),$(eval $(call compile_s,$(call rt_object,$(source)),$(source))))
$(eval $(call compile_c,$(call app_object,main.c),main.c))
$(eval $(call compile_c,$(call app_object,$(T89_KEY_SOURCE)),$(T89_KEY_SOURCE)))
$(foreach source,$(T89_LCD_SOURCES),$(eval $(call compile_c,$(call app_object,$(source)),$(source))))

$(BUILD)/flash.raw: FORCE
	@test -n "$(OS_IMAGE)" || { echo "set OS_IMAGE=<private.raw>" >&2; exit 1; }
	@mkdir -p $(dir $@)
	@cmp -s "$(OS_IMAGE)" $@ || cp "$(OS_IMAGE)" $@

$(call app_object,flash.S): flash.S $(BUILD)/flash.raw $(MAKEFILE_LIST)
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -DT89_FLASH_FILE='"$(BUILD)/flash.raw"' -c -o $@ $<

clean:
	rm -rf $(BUILD)

-include $(CORE_OBJECTS:.o=.d) $(RT_OBJECTS:.o=.d) $(APP_OBJECTS:.o=.d)

.PHONY: all clean FORCE
