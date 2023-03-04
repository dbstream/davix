/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_TIME_H
#define __DAVIX_TIME_H

#define TIMER_HZ 100 /* How often is timer_interrupt() called? */

#include <davix/types.h>

struct logical_cpu;

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

/*
 * Timer interrupt, called periodically by arch.
 * The exact time interval does not matter, as the scheduler/timer subsystem
 * uses ns_since_boot() to determine how much time has passed.
 */
void timer_interrupt(void);

/*
 * This is a *really* low-level timer API.
 *
 * A "raw" timer is a timer which simply calls a function when it expires. These
 * are kept in CPU-local lists. Usually, you will want to use a higher-level API
 * that builds upon raw timers, such as sched timers.
 */
struct raw_timer {
	struct avlnode node;
	struct logical_cpu *on_cpu;
	void (*fn)(struct raw_timer *);
	nsecs_t expiry_at;
	bool expired;
};

/*
 * Register/deregister a raw timer.
 */
void register_timer(struct raw_timer *timer);
void deregister_timer(struct raw_timer *timer);

/*
 * Sched timers are a high-level timer API that allow you to block the current
 * task until a timer expires. These are meant to live on the stack and only
 * work for a single task.
 *
 * Possible usage includes:
 *   Time-outs for mutex & semaphore acquiring.
 *   Sleeping for some specified time.
 */
struct sched_timer {
	struct raw_timer raw;
	struct task *task;
};

/*
 * Create a sched timer that expires at 'at'.
 */
void create_sched_timer(struct sched_timer *timer, nsecs_t at);

/*
 * Wait for a timer at the current task state. This always returns with
 * TASK_RUNNING as the current task state.
 *
 * May return for a number of reasons, namely:
 *   1) Timer expired.
 *   2) Task was interrupted by a signal.
 *
 * Return value: did the timer expire?
 */
bool sched_timer_wait(struct sched_timer *timer);

/*
 * Destroy a sched timer.
 */
void destroy_sched_timer(struct sched_timer *timer);

#endif /* __DAVIX_TIME_H */
