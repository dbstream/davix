/**
 * Kernel mutexes.
 * Copyright (C) 2025-present  dbstream
 *
 * NOTE: the interruptible and timeout variants of Mutex::lock return zero
 * indicating success.
 */
#pragma once

#include <davix/sched.h>
#include <davix/time.h>
#include <dsl/list.h>
#include <stdint.h>

struct MutexWaiter {
	dsl::ListHead entry;
	Task *task;
	sched_ticket_t ticket;
};

typedef dsl::TypedList<MutexWaiter, &MutexWaiter::entry> MutexWaiterList;

struct Mutex {
	uintptr_t m_owner_and_flags = 0;
	MutexWaiterList m_waiters;

	constexpr void
	init (void)
	{
		m_owner_and_flags = 0;
		m_waiters.init ();
	}

	void
	unlock (void);

	bool
	trylock (void);

	void
	lock (void);

	/* NOTE: zero means success */
	int
	lock_interruptible (void);

	/* NOTE: zero means success */
	int
	lock_timeout (nsecs_t ns);

	/* NOTE: zero means success */
	int
	lock_timeout_interruptible (nsecs_t ns);
};

