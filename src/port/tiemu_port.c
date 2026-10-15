/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "images.h"
#include "ti68k_def.h"

IMG_INFO img_infos;
int img_loaded;
int img_changed;

Ti68kParameters params;
Ti68kHardware tihw;
Ti68kLinkPort linkp;
Ti68kBreakpoints bkpts;
Ti68kLogging logger;

CableHandle *cable_handle;
CalcHandle *calc_handle;
int recfile_flag;

static void dbus_reinit_stub(void) {}
static void dbus_putbyte_stub(uint8_t value) { (void)value; }
static uint8_t dbus_getbyte_stub(void) { return 0; }
static int dbus_checkread_stub(void) { return 0; }

void (*hw_dbus_reinit)(void) = dbus_reinit_stub;
void (*hw_dbus_putbyte)(uint8_t) = dbus_putbyte_stub;
uint8_t (*hw_dbus_getbyte)(void) = dbus_getbyte_stub;
int (*hw_dbus_checkread)(void) = dbus_checkread_stub;

int hw_dbus_init(void) { return 0; }
int hw_dbus_reset(void) { return 0; }
int hw_dbus_exit(void) { return 0; }
int recfile(void) { recfile_flag = 0; return 0; }
void dbgbkpts_refresh_window(void) {}

uint32_t heap_deref(int handle) { (void)handle; return 0; }

static void log_message(const char *level, const char *format, va_list args)
{
#ifndef TINY89_SILENT_LOG
    fprintf(stderr, "[%s] ", level);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
#else
    (void)level;
    (void)format;
    (void)args;
#endif
}

#define DEFINE_LOG(name, level)                         \
    void name(const char *format, ...)                  \
    {                                                   \
        va_list args;                                   \
        va_start(args, format);                         \
        log_message(level, format, args);               \
        va_end(args);                                   \
    }

DEFINE_LOG(tiemu_debug, "debug")
DEFINE_LOG(tiemu_info, "info")
DEFINE_LOG(tiemu_message, "message")
DEFINE_LOG(tiemu_warning, "warning")
DEFINE_LOG(tiemu_critical, "critical")
DEFINE_LOG(tiemu_error, "error")

void tiemu_err(int error, const char *format, ...)
{
    (void)error;
    (void)format;
}

static int calc_index(int calc_type)
{
    int index = 0;
    while (calc_type > 1) {
        calc_type >>= 1;
        index++;
    }
    return index;
}

int ti68k_get_rom_size(int calc_type)
{
    static const int sizes[] = { 1024 * 1024, 2 * 1024 * 1024,
        2 * 1024 * 1024, 4 * 1024 * 1024, 4 * 1024 * 1024 };
    return sizes[calc_index(calc_type)];
}

int ti68k_get_ram_size(int calc_type)
{
    (void)calc_type;
    return 256 * 1024;
}

int ti68k_get_io_size(int calc_type) { (void)calc_type; return 32; }
int ti68k_get_io2_size(int calc_type) { return calc_type == TI89t ? 256 : 32; }
int ti68k_get_io3_size(int calc_type) { return calc_type == TI89t ? 256 : 0; }
