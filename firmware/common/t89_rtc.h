/* SPDX-License-Identifier: MIT */
#ifndef T89_RTC_H
#define T89_RTC_H

#include <stdint.h>

/*
 * The board's clock, as TinyTang hands it over at launch: UTC seconds since
 * 1970 and the desktop's zone in minutes east of UTC, in the settings blob the
 * program asks for before it enters link mode.
 *
 * Why this exists.  The calculator's clock chip is not fitted in this fabric,
 * and TiEmu reads the host clock when the calculator asks the time
 * (core/ti_hw/rtc_hw3.c's rtc3_get_time calls gettimeofday).  Before this
 * module that answer was seconds since the board reset -- a small count, not a
 * date -- so the calculator's own clock arithmetic ran from wherever that put
 * it, and a machine restored from a state file came back to a date that had
 * moved.  With real seconds behind it the same arithmetic is correct: rtc3
 * subtracts the 1997 reference epoch and hands the calculator the count its
 * hardware would give, and a restored `rtc3_beg`/`rtc3_load` pair then keeps
 * the clock running across a power cycle the way a battery-backed calculator
 * does.
 *
 * One reading is enough.  The seconds between launches are counted here from
 * the platform's cycle counter, which is crystal-derived, so a session's drift
 * is parts per million rather than something a person would notice.  A board
 * that does not know the time answers zero, which seeds nothing: the caller
 * keeps the platform count instead, and the calculator is never told a date
 * nobody sent.
 */

typedef struct {
    uint32_t utc;        /* UTC seconds at the moment of `cycles` */
    int32_t  zone_min;   /* the desktop's zone, minutes east of UTC */
    uint64_t cycles;     /* the cycle count when the reading was taken */
    int      synced;     /* 0 until a reading has arrived */
} t89_rtc_t;

/* Fold a reading in.  Returns 1 when it was taken, 0 when there is no clock to
 * take: a zero `utc` is the board's "never set" answer, not 1970.  Pure: the
 * host test exercises exactly what the board runs. */
int t89_rtc_set(t89_rtc_t *rtc, uint32_t utc, int32_t zone_min, uint64_t cycles);

/* The wall clock the desktop shows: the reading, the zone applied, and the
 * whole seconds counted since it.  Seconds and microseconds since 1970-01-01,
 * the shape gettimeofday wants.  Returns 0 when no reading has arrived. */
int t89_rtc_local(const t89_rtc_t *rtc, uint64_t cycles, uint32_t hz,
                  uint64_t *seconds, uint32_t *micros);

/* The board's copy: seed it from the launch blob, and answer the clock for
 * gettimeofday until it is reset. */
void t89_rtc_reset(void);
int t89_rtc_seed(uint32_t utc, int32_t zone_min, uint64_t cycles);
int t89_rtc_now(uint64_t *seconds, uint32_t *micros);

#endif
