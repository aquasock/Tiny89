/* SPDX-License-Identifier: MIT */
#include "t89_rtc.h"

#include <stddef.h>

/*
 * The cycle counter, and the host test's stand-in for it.
 *
 * The test cannot use the board's counter: t89_cycles64() is a CSR read that
 * will not assemble on a host.  Everything else here -- the reading, the zone,
 * the count since then -- is the board's own code under test.
 */
#ifdef T89_RTC_HOST
uint64_t t89_rtc_hw_cycles(void);
#define RTC_CYCLES() (t89_rtc_hw_cycles())
#define RTC_HZ       750000000u
#else
#include "tiny89_platform.h"
#define RTC_CYCLES() (t89_cycles64())
#define RTC_HZ       T89_CORE_HZ
#endif

/* The board's reading, seeded at launch and read by t89_rtc_now(). */
static t89_rtc_t s_rtc;

int t89_rtc_set(t89_rtc_t *rtc, uint32_t utc, int32_t zone_min, uint64_t cycles)
{
    if (utc == 0u) {
        /* The board's answer when its RTC has never been set.  A clock of 1970
         * that nobody sent would be worse than no clock at all. */
        return 0;
    }
    rtc->utc = utc;
    rtc->zone_min = zone_min;
    rtc->cycles = cycles;
    rtc->synced = 1;
    return 1;
}

int t89_rtc_local(const t89_rtc_t *rtc, uint64_t cycles, uint32_t hz,
                  uint64_t *seconds, uint32_t *micros)
{
    uint64_t elapsed;

    if (!rtc->synced || hz == 0u) {
        return 0;
    }
    /* The count since the reading.  It cannot run backwards: the platform's
     * counter is 64-bit and monotonic, and a reading taken after this call has
     * a smaller elapsed time, which is clamped rather than subtracted. */
    elapsed = cycles > rtc->cycles ? cycles - rtc->cycles : 0u;

    if (seconds != NULL) {
        *seconds = (uint64_t)(int64_t)rtc->utc +
                   (uint64_t)((int64_t)rtc->zone_min * 60) +
                   elapsed / hz;
    }
    if (micros != NULL) {
        *micros = (uint32_t)((elapsed % hz) * 1000000u / hz);
    }
    return 1;
}

void t89_rtc_reset(void)
{
    s_rtc.utc = 0u;
    s_rtc.zone_min = 0;
    s_rtc.cycles = 0u;
    s_rtc.synced = 0;
}

int t89_rtc_seed(uint32_t utc, int32_t zone_min, uint64_t cycles)
{
    return t89_rtc_set(&s_rtc, utc, zone_min, cycles);
}

int t89_rtc_now(uint64_t *seconds, uint32_t *micros)
{
    return t89_rtc_local(&s_rtc, RTC_CYCLES(), RTC_HZ, seconds, micros);
}
