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
