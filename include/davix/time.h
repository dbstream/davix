/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_TIME_H
#define __DAVIX_TIME_H

/*
 * Why 'unsigned long long'?
 *
 * On some systems that the kernel *might* be ported to, 'unsigned long' can be
 * 32-bit, which means that the largest time quanta supported with nanosecond
 * accuracy would be <5s.
 */
typedef unsigned long long nsecs_t; /* nanoseconds */
typedef unsigned long long usecs_t; /* microseconds */
typedef unsigned long long msecs_t; /* milliseconds */

/*
 * How much time has passed since boot?
 *
 * NOTE: There is no guarantee that this will progress forwards between
 * different CPUs, and the kernel shouldn't depend on that behavior.
 */
nsecs_t ns_since_boot(void);

static inline usecs_t us_since_boot(void)
{
	return (usecs_t) ns_since_boot() / 1000;
}

static inline msecs_t ms_since_boot(void)
{
	return (msecs_t) ns_since_boot() / 1000000;
}

/*
 * Stall the kernel for some time period.
 *
 * Please keep these time periods as short as possible.
 */
void ndelay(nsecs_t ns);

static inline void udelay(usecs_t us)
{
	ndelay((nsecs_t) us * 1000);
}

static inline void mdelay(msecs_t ms)
{
	ndelay((nsecs_t) ms * 1000000);
}

/*
 * Initialize the basic time services, such as stalling and getting the time
 * since boot.
 */
void time_init(void);

#endif /* __DAVIX_TIME_H */
