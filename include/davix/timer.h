/**
 * Timer events.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_TIMER_H
#define _DAVIX_TIMER_H 1

/**
 *   Kernel timers (ktimer)
 *
 * A ktimer contains the following state:
 *   - Timer expiration time.
 *   - Callback function.
 *   - A flag indicating if the timer has been triggered or not.
 *
 * ktimers are CPU-local. Each CPU maintains a sorted queue of timers.
 * ktimer_add and ktimer_remove work on the current CPU.
 */

#include <davix/avltree_types.h>
#include <davix/list.h>
#include <davix/time.h>

struct ktimer {
	struct avl_node node;
	nsecs_t expiry;
	void (*callback)(struct ktimer *);
	bool triggered;
	unsigned int cpu;
};

/**
 * Register a ktimer on the current CPU.
 */
extern void
ktimer_add (struct ktimer *timer);

/**
 * Unregister a ktimer that was registered on the current CPU. If the timer is
 * already removed due to being triggered, returns true.
 */
extern bool
ktimer_remove (struct ktimer *timer);

/**
 * Handle a timer tick on the current CPU. Should be called by the timer
 * interrupt handler (e.g. APIC on x86 systems).
 */
extern void
local_timer_tick (void);

#endif /* _DAVIX_TIMER_H */
