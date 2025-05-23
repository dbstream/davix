/**
 * ktimers
 * Copyright (C) 2025-present  dbstream
 *
 * Ktimers are simple callback-based percpu timer events.
 */
#pragma once

#include <davix/time.h>
#include <dsl/avltree.h>

/**
 * A KTimer has the following state:
 * - Expiry time in nanoseconds since boot.
 * - Callback function and callback argument.
 * - A flag indicating whether the timer is currently on the queue.
 *
 * Ktimers are percpu.  Each CPU maintains an ordered queue of timers.
 * Importantly, KTimer::enqueue and KTimer::remove must be called on the same
 * CPU.
 *
 * The callback will be invoked on the same CPU that enqueued the timer, and in
 * DPC context.
 */
struct KTimer {
	dsl::AVLNode tree_node;
	bool on_queue = false;
	nsecs_t expiry_ns;
	void (*callback_fn) (KTimer *, void *);
	void *callback_arg;

	constexpr void
	init (void (*fn) (KTimer *, void *), void *arg)
	{
		callback_fn = fn;
		callback_arg = arg;
		on_queue = false;
	}

	/** Returns true if the KTimer was already on the queue.  */
	bool
	enqueue (nsecs_t t);

	/** Returns false if the KTimer was not on a queue.  */
	bool
	remove (void);
};

void
ktimer_handle_timer_interrupt (void);
