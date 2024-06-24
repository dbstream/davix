/**
 * 'struct timespec' and kernel timekeeping
 * Copyright (C) 2024  dbstream
 *
 * This file provides functions for setting and querying time. We differentiate
 * between two types of time:
 *   - Kernel time. This is a monotonic timer that starts at 0 when the system
 *     boots. It cannot be changed.
 *   - Calendar time. This counts time since epoch. It is derived from an offset
 *     calculation on kernel time.
 *
 * TODO: implement calendar time.
 */
#ifndef DAVIX_TIME_H
#define DAVIX_TIME_H 1

struct timespec {
	long long	tv_sec;
	long		tv_nsec;
};

typedef long long time_t;

typedef unsigned long long nsecs_t;
typedef unsigned long long usecs_t;
typedef unsigned long long msecs_t;

extern nsecs_t
ns_since_boot (void);

static inline usecs_t
us_since_boot (void)
{
	return ns_since_boot () / 1000;
}

static inline msecs_t
ms_since_boot (void)
{
	return ns_since_boot () / 1000000;
}

#endif /* DAVIX_TIME_H */
