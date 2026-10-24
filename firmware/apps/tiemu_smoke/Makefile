# SPDX-License-Identifier: GPL-2.0-or-later
#
# Tiny89's AE350 program: the emulated calculator and its console report.
#
#   make           the program, build/firmware/apps/tiemu_smoke/
#   make DIAG=1    it with the per-second heartbeat instead of one a minute,
#                  build/firmware/apps/tiemu_smoke_diag/
#   make BINARY=1  it with the save's payload sent as the bytes themselves
#                  instead of hex lines -- four times faster, and only sound
#                  while nothing is lost on the link; see main.c's save_ram()
#
# The default build reports once a minute, the way TinyNES's board build does
# (main.c: HEARTBEAT_FRAMES).  DIAG=1 is TinyNES's flag for a build that says
# more, and here it changes that one interval and nothing else.
#
# BINARY=1 exists because the byte payload is measured rather than trusted.  It
# is 16.6 s against 65.8 s for the same 256 KiB on this board, and one of two
# runs of it came back with every byte present and the CRC disagreeing -- which
# is what a lost frame does to a payload with no line boundaries to re-align on
# (main.c's save_ram(), core-log entry 15).  The default is hex until that loss
# is found, and this flag is how the measurement is repeated.

ROOT       := $(abspath ../../..)
ifeq ($(DIAG),1)
BUILD      ?= $(ROOT)/build/firmware/apps/tiemu_smoke_diag
else ifeq ($(BINARY),1)
BUILD      ?= $(ROOT)/build/firmware/apps/tiemu_smoke_binary
else
BUILD      ?= $(ROOT)/build/firmware/apps/tiemu_smoke
endif
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
              -I$(ROOT)/firmware/boot -I$(ROOT)/firmware/boot/support \
              -I$(TIEMU_SRC) -I$(TIEMU_SRC)/core -I$(UAE) \
              -I$(TIEMU_SRC)/core/ti_hw -I$(TIEMU_SRC)/core/ti_sw \
              -I$(TIEMU_SRC)/core/dbg -I$(TIEMU_SRC)/misc
APP_DEFS   := $(if $(filter 1,$(DIAG)),-DT89_DIAG)
APP_DEFS   += $(if $(filter 1,$(BINARY)),-DT89_SAVE_BINARY)
APP_DEFS   += $(if $(filter 1,$(HEX)),-DT89_SAVE_HEX)
APP_DEFS   += $(if $(filter 1,$(AUTOSAVE)),-DT89_AUTOSAVE)
APP_DEFS   += $(if $(filter 1,$(LINKTRACE)),-DT89_LINK_TRACE)
APP_DEFS   += $(if $(filter 1,$(LINKSEND)),-DT89_LINKSEND)
CPPFLAGS   += $(APP_DEFS)
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
T89_SAVE_SOURCE := $(ROOT)/firmware/common/t89_save.c
T89_LCD_SOURCES := $(ROOT)/firmware/common/t89_lcd.c \
                   $(ROOT)/firmware/common/t89_lcd_gray.c
T89_LINK_SOURCE := $(ROOT)/firmware/common/t89_link.c
T89_PROTO_SOURCE := $(ROOT)/firmware/common/t89_proto.c
T89_FILE_SOURCE := $(ROOT)/firmware/common/t89_file.c
APP_SOURCES := main.c $(ROOT)/firmware/boot/load.c \
               $(ROOT)/firmware/boot/support/uart.c \
               $(ROOT)/firmware/common/t89_bios.c \
               $(T89_KEY_SOURCE) $(T89_SAVE_SOURCE) $(T89_LCD_SOURCES) \
               $(T89_LINK_SOURCE) $(T89_PROTO_SOURCE) $(T89_FILE_SOURCE)

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
$(eval $(call compile_c,$(call app_object,$(ROOT)/firmware/boot/load.c),$(ROOT)/firmware/boot/load.c))
$(eval $(call compile_c,$(call app_object,$(ROOT)/firmware/boot/support/uart.c),$(ROOT)/firmware/boot/support/uart.c))
$(eval $(call compile_c,$(call app_object,$(ROOT)/firmware/common/t89_bios.c),$(ROOT)/firmware/common/t89_bios.c))
$(eval $(call compile_c,$(call app_object,$(T89_KEY_SOURCE)),$(T89_KEY_SOURCE)))
$(foreach source,$(T89_LCD_SOURCES),$(eval $(call compile_c,$(call app_object,$(source)),$(source))))
$(eval $(call compile_c,$(call app_object,$(T89_SAVE_SOURCE)),$(T89_SAVE_SOURCE)))
$(eval $(call compile_c,$(call app_object,$(T89_LINK_SOURCE)),$(T89_LINK_SOURCE)))
$(eval $(call compile_c,$(call app_object,$(T89_PROTO_SOURCE)),$(T89_PROTO_SOURCE)))
$(eval $(call compile_c,$(call app_object,$(T89_FILE_SOURCE)),$(T89_FILE_SOURCE)))

# DIAG changes a define and nothing else, so main.c's object would otherwise
# survive a change to it: the options are kept in a file that is rewritten
# only when they change, and main.c's object depends on it.  TinyNES's
# Makefile keeps the same guard around its PLAY and DIAG builds, but writes
# the file only when the values differ -- which leaves the dependency missing
# on the first build of a tree that defines nothing, so the file is created
# here when it does not exist as well.
DEFS_STAMP := $(BUILD)/app_defs
ifeq ($(wildcard $(DEFS_STAMP)),)
$(shell mkdir -p $(BUILD))
$(file >$(DEFS_STAMP),$(APP_DEFS))
else ifneq ($(file <$(DEFS_STAMP)),$(APP_DEFS))
$(file >$(DEFS_STAMP),$(APP_DEFS))
endif
$(call app_object,main.c): $(DEFS_STAMP)

clean:
	rm -rf $(BUILD)

-include $(CORE_OBJECTS:.o=.d) $(RT_OBJECTS:.o=.d) $(APP_OBJECTS:.o=.d)

.PHONY: all clean FORCE
