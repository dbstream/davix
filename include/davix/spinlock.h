/**
 * Spinlocks.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <asm/irql.h>
#include <davix/atomic.h>

struct spinlock_t {
	unsigned char value = 0;

	inline void
	init (void)
	{
		value = 0;
	}

	inline bool
	raw_trylock (void)
	{
		if (!atomic_exchange_acquire (&value, true)) [[likely]] {
			return true;
		} else
			return false;
	}

	inline void
	raw_lock (void)
	{
		for (;;) {
			if (raw_trylock ())
				break;

			do
				smp_spinlock_hint ();
			while (atomic_load_relaxed (&value));
		}
	}

	inline void
	raw_unlock (void)
	{
		atomic_store_release (&value, 0);
	}

	inline irql_cookie_t
	lock (irql_t irql)
	{
		irql_cookie_t cookie = raise_irql (irql);
		raw_lock ();
		return cookie;
	}

	inline void
	unlock (irql_cookie_t cookie)
	{
		raw_unlock ();
		lower_irql (cookie);
	}
};

class scoped_spinlock {
	spinlock_t *m_lock;
	irql_cookie_t m_cookie;

public:
	inline
	scoped_spinlock (spinlock_t &lock, irql_t irql)
		: m_lock (&lock)
		, m_cookie (lock.lock (irql))
	{}

	inline
	~scoped_spinlock (void)
	{
		m_lock->unlock (m_cookie);
	}

	scoped_spinlock (void) = delete;
	scoped_spinlock (scoped_spinlock &) = delete;
	scoped_spinlock (scoped_spinlock &&) = delete;
	scoped_spinlock &operator= (scoped_spinlock &&) = delete;
};
