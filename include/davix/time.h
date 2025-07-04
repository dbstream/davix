/**
 * In-kernel timekeeping.
 * Copyright (C) 2025-present  dbstream
 *
 * This file provides functions for querying kernel time, which is a monotonic
 * timer that starts at zero when the system boots.
 */
#pragma once

typedef unsigned long long nsecs_t;
typedef unsigned long long usecs_t;
typedef unsigned long long msecs_t;

/*
 * NO_TIMEOUT: highest possible value for nsecs_t, usually taken by functions
 * with a timeout to indicate that no timeout should be used.
 */
constexpr static nsecs_t NO_TIMEOUT = -1ULL;

/**
 * ns_since_boot - query the number of nanoseconds that have passed since boot.
 */
nsecs_t
ns_since_boot (void);

/**
 * us_since_boot - query the number of microseconds that have passed since boot.
 */
static inline usecs_t
us_since_boot (void)
{
	return ns_since_boot () / 1000;
}

/**
 * ms_since_boot - query the number of milliseconds that have passed since boot.
 */
static inline msecs_t
ms_since_boot (void)
{
	return ns_since_boot () / 1000000;
}

/**
 * ndelay - stall the CPU.
 * @ns: nanoseconds to stall for
 */
void
ndelay (nsecs_t ns);

/**
 * udelay - stall the CPU.
 * @us: microseconds to stall for
 */
static inline void
udelay (usecs_t us)
{
	ndelay (us * 1000);
}

/**
 * mdelay - stall the CPU.
 * @ms: milliseconds to stall for
 */
static inline void
mdelay (msecs_t ms)
{
	ndelay (ms * 1000000);
}
