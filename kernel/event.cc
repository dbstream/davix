/**
 * Kernel event waiting.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/atomic.h>
#include <davix/event.h>
#include <davix/sched.h>

void
KEvent::wait (void)
{
retry:
	if (atomic_load_acquire (&value))
		return;

	lock.lock_dpc ();
	if (value) {
		lock.unlock_dpc ();
		return;
	}

	KEventWaiter waiter;
	waiter.task = get_current_task ();
	waiter.ticket = sched_get_blocking_ticket ();
	waiter.on_list = true;
	waiters.push_back (&waiter);

	set_current_state (TASK_UNINTERRUPTIBLE);
	lock.raw_unlock ();
	schedule ();

	if (!atomic_load_acquire (&waiter.on_list)) {
		enable_dpc ();
		return;
	}

	lock.raw_lock ();
	if (waiter.on_list)
		waiter.list.remove ();
	lock.unlock_dpc ();
	/*
	 * In case we erronerously woke up from something else than completion:
	 */
	goto retry;
}

void
KEvent::set (void)
{
	lock.lock_dpc ();
	atomic_store_release (&value, 1);
	while (!waiters.empty ()) {
		KEventWaiter *waiter = waiters.pop_front ();
		Task *task = waiter->task;
		sched_ticket_t ticket = waiter->ticket;
		atomic_store_relaxed (&waiter->on_list, false);
		lock.unlock_dpc ();
		sched_wake (task, ticket);
		lock.lock_dpc ();
	}
	lock.unlock_dpc ();
}

