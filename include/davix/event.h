/**
 * Kernel event waiting.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <davix/sched.h>
#include <davix/spinlock.h>
#include <dsl/list.h>

struct Task;

struct KEventWaiter {
	dsl::ListHead list;
	Task *task;
	sched_ticket_t ticket;
	bool on_list;
};

typedef dsl::TypedList<KEventWaiter, &KEventWaiter::list> KEventWaitList;

struct KEvent {
	KEventWaitList waiters;
	spinlock_t lock;
	int value = 0;

	constexpr void
	init (void)
	{
		waiters.init ();
		lock.init ();
		value = 0;
	}

	void
	wait (void);

	void
	set (void);
};

