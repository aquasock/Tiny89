/* SPDX-License-Identifier: MIT */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "tiny89_platform.h"
#include "t89_rtc.h"

#undef errno
extern int errno;
extern char _end[], __heap_end[];

void t89_rt_reboot(void) __attribute__((noreturn));

int _write(int fd, const char *buffer, int length)
{
    int index;
    if (fd != 1 && fd != 2) { errno = EBADF; return -1; }
    for (index = 0; index < length; index++) {
        if (buffer[index] == '\n') t89_put_char('\r');
        t89_put_char(buffer[index]);
    }
    return length;
}

void *_sbrk(ptrdiff_t increment)
{
    static char *current = _end;
    char *old = current;
    if (increment > __heap_end - current || increment < _end - current) {
        errno = ENOMEM;
        return (void *)-1;
    }
    current += increment;
    return old;
}

/* The emulator asks this for the calculator's clock (TiEmu's rtc3_get_time
 * calls gettimeofday), so the answer has to be a real date: the board's RTC,
 * carried to the core by TinyTang's time frame and counted forward here from
 * the cycle counter between frames.  Before a reading has arrived -- a core
 * just loaded, the desk layer not yet polling -- the platform's own count is
 * the fallback, which is what this returned before the frame existed.  It is
 * not a date, and the calculator is told the truth as soon as one arrives;
 * the alternative, inventing a date, is worse than a clock that is briefly
 * wrong in a way its own boot has already finished with. */
int _gettimeofday(struct timeval *tv, void *timezone)
{
    uint64_t seconds;
    uint32_t micros;
    (void)timezone;

    if (tv != NULL) {
        if (!t89_rtc_now(&seconds, &micros)) {
            const uint64_t cycles = t89_cycles64();
            seconds = cycles / T89_CORE_HZ;
            micros = (uint32_t)((cycles % T89_CORE_HZ) / (T89_CORE_HZ / 1000000u));
        }
        tv->tv_sec = (time_t)seconds;
        tv->tv_usec = (suseconds_t)micros;
    }
    return 0;
}

int _close(int fd) { (void)fd; errno = EBADF; return -1; }
int _getpid(void) { return 1; }
int _kill(int pid, int signal) { (void)pid; (void)signal; errno = EINVAL; return -1; }
int _lseek(int fd, int offset, int whence)
{ (void)fd; (void)offset; (void)whence; errno = ESPIPE; return -1; }
int _read(int fd, char *buffer, int length)
{ (void)fd; (void)buffer; (void)length; errno = EBADF; return -1; }
int _isatty(int fd) { return fd >= 0 && fd <= 2; }
int _fstat(int fd, struct stat *status)
{
    if (fd < 0 || fd > 2) { errno = EBADF; return -1; }
    status->st_mode = S_IFCHR;
    return 0;
}
int _stat(const char *path, struct stat *status)
{ (void)path; (void)status; errno = ENOENT; return -1; }

void _exit(int code)
{
    char digit = (char)('0' + (code >= 0 && code <= 9 ? code : 9));
    t89_put_string("TinyNES EXIT ");
    t89_put_char(digit);
    t89_put_string("\r\n");
    t89_wait_tx_idle();
    t89_rt_reboot();
}

void t89_rt_trap(uint32_t cause, uint32_t epc, uint32_t value)
{
    static const char hex[] = "0123456789abcdef";
    uint32_t fields[] = { cause, epc, value };
    unsigned field, shift;
    t89_put_string("BAD Tiny89 TRAP");
    for (field = 0; field < 3; field++) {
        t89_put_char(' ');
        for (shift = 28; shift < 32; shift -= 4)
            t89_put_char(hex[(fields[field] >> shift) & 15]);
    }
    t89_put_string("\r\n");
    t89_wait_tx_idle();
    t89_rt_reboot();
}
