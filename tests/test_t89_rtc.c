/*
 * Host test for the board's clock as the calculator sees it.
 *
 * What the module can get wrong is arithmetic, not hardware: the zone's sign,
 * the seconds counted since the reading, a reading that arrives with no clock
 * behind it, and the refusal to answer at all before one does -- that last one
 * matters because gettimeofday's fallback is a count since reset, and a
 * calculator told the epoch by us would be worse than one left alone.
 *
 * The cycle counter is a stand-in, supplied through t89_rtc.c's T89_RTC_HOST
 * branch, so seed() and now() here are the board's own code.  The board's half
 * of the same blob -- the byte order the firmware writes -- is TinyTang's
 * `Tiny89 SETTINGS WANT` handler, and TinyTang's own suite covers its clock.
 *
 *   tests/run_t89_rtc_test.sh
 */
#include <stdint.h>
#include <stdio.h>

#include "t89_rtc.h"

static int checks;
static int fails;

static void check(int ok, const char *what)
{
    checks++;
    if (!ok) {
        fails++;
        printf("  FAIL: %s\n", what);
    }
}

static void check_u64(uint64_t got, uint64_t want, const char *what)
{
    checks++;
    if (got != want) {
        fails++;
        printf("  FAIL: %s: got %llu want %llu\n", what,
               (unsigned long long)got, (unsigned long long)want);
    }
}

/* ---- the cycle counter, as the platform would report it ---- */
static uint64_t g_cycles;
uint64_t t89_rtc_hw_cycles(void) { return g_cycles; }

/* 2026-09-27T12:00:00Z, which is 05:00 in Phoenix. */
#define T0_UTC 1790001600u
#define PHOENIX_MIN (-420)          /* -7 h */
#define HZ 750000000u
/* Cycles in a whole number of seconds: the multiply has to be 64-bit, since
 * 61 * 750000000 does not fit the 32-bit unsigned the literal would otherwise
 * have, and a wrapped count reads as a clock that barely moved. */
#define SEC(n) ((uint64_t)(n) * HZ)

int main(void)
{
    t89_rtc_t rtc;
    uint64_t secs;
    uint32_t micros;

    /* ---- nothing has arrived: no answer, so gettimeofday falls back ---- */
    t89_rtc_reset();
    check(t89_rtc_now(&secs, &micros) == 0,
          "an unseeded clock answered anyway");

    /* ---- a board that has never set its RTC seeds nothing ---- */
    check(t89_rtc_seed(0u, PHOENIX_MIN, 0u) == 0,
          "a zero reading was taken as the epoch");
    check(t89_rtc_now(&secs, &micros) == 0,
          "a zero reading left the clock answering");

    /* ---- a reading is taken whole, zone and all ---- */
    g_cycles = 0u;
    check(t89_rtc_seed(T0_UTC, PHOENIX_MIN, g_cycles) == 1,
          "the first reading was refused");
    check(t89_rtc_now(&secs, &micros) == 1, "a seeded clock refused to answer");
    check_u64(secs, (uint64_t)T0_UTC + (uint64_t)(PHOENIX_MIN * 60),
              "the zone is not applied to the reading");
    check_u64(micros, 0u, "the microseconds at the moment of the reading");

    /* ---- the seconds since the reading are counted here ---- */
    g_cycles = SEC(1);
    check(t89_rtc_now(&secs, &micros) == 1, "a tick was refused");
    check_u64(secs, (uint64_t)T0_UTC + (uint64_t)(PHOENIX_MIN * 60) + 1u,
              "one second of cycles is not one second of clock");

    g_cycles = HZ / 2u;
    check(t89_rtc_now(&secs, &micros) == 1, "a half second was refused");
    check_u64(micros, 500000u, "half a second of cycles is not 500000 us");

    g_cycles = SEC(61) + HZ / 4u;
    check_u64((t89_rtc_now(&secs, &micros), secs),
              (uint64_t)T0_UTC + (uint64_t)(PHOENIX_MIN * 60) + 61u,
              "61 seconds of cycles are not 61 seconds of clock");
    check_u64(micros, 250000u,
              "the microseconds of a part second are not carried");

    /* ---- an eastern zone is positive, a western one negative ---- */
    g_cycles = 0u;
    check(t89_rtc_seed(T0_UTC, 330, g_cycles) == 1, "an eastern zone was refused");
    check_u64((t89_rtc_now(&secs, &micros), secs),
              (uint64_t)T0_UTC + 330u * 60u, "an eastern zone was subtracted");

    check(t89_rtc_seed(T0_UTC, -330, g_cycles) == 1, "a western zone was refused");
    check_u64((t89_rtc_now(&secs, &micros), secs),
              (uint64_t)T0_UTC - 330u * 60u, "a western zone was added");

    /* ---- a zone west of UTC can straddle midnight ---- */
    check(t89_rtc_seed(T0_UTC - 60u, PHOENIX_MIN, 0u) == 1,
          "a reading near midnight was refused");
    check_u64((t89_rtc_now(&secs, &micros), secs),
              (uint64_t)T0_UTC - 60u - 420u * 60u,
              "a western zone was clamped at midnight");

    /* ---- a reading cannot run the clock backwards ---- */
    g_cycles = SEC(10);
    check(t89_rtc_seed(T0_UTC + 240u, 0, SEC(12)) == 1,
          "a reading taken later was refused");
    g_cycles = SEC(11);                  /* the counter is behind the reading */
    check(t89_rtc_now(&secs, &micros) == 1, "a caller behind the reading was refused");
    check_u64(secs, (uint64_t)T0_UTC + 240u,
              "a reading dated after the caller ran the clock backwards");

    /* ---- the answer is a date, not a count since reset ---- */
    check(secs > 1000000000ull,
          "the clock is not a real date: the calculator would read a small count");

    /* ---- reset forgets the reading ---- */
    t89_rtc_reset();
    check(t89_rtc_now(&secs, &micros) == 0, "reset left the clock answering");

    /* ---- a zero rate cannot divide, and is refused rather than trapped ---- */
    rtc.utc = T0_UTC;
    rtc.zone_min = 0;
    rtc.cycles = 0u;
    rtc.synced = 1;
    check(!t89_rtc_local(&rtc, 0u, 0u, &secs, &micros),
          "a zero rate was accepted");

    printf("t89_rtc: %s (%d checks)\n", fails == 0 ? "PASS" : "FAIL", checks);
    return fails == 0 ? 0 : 1;
}
