/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TINY89_PORT_LOGGING_H
#define TINY89_PORT_LOGGING_H

#include "intl.h"

void tiemu_debug(const char *format, ...);
void tiemu_info(const char *format, ...);
void tiemu_message(const char *format, ...);
void tiemu_warning(const char *format, ...);
void tiemu_critical(const char *format, ...);
void tiemu_error(const char *format, ...);

#endif
