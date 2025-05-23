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

	inline void
	lock_dpc (void)
	{
		disable_dpc ();
		raw_lock ();
	}

	inline void
	unlock_dpc (void)
	{
		raw_unlock ();
		enable_dpc ();
	}

	inline void
	lock_irq (void)
	{
		disable_dpc ();
		disable_irq ();
		raw_lock ();
	}

	inline void
	unlock_irq (void)
	{
		raw_unlock ();
		enable_irq ();
		enable_dpc ();
	}
};

class scoped_spinlock_dpc {
	spinlock_t *m_lock;

public:
	inline
	scoped_spinlock_dpc (spinlock_t &lock)
		: m_lock (&lock)
	{
		m_lock->lock_dpc ();
	}

	inline
	~scoped_spinlock_dpc (void)
	{
		m_lock->unlock_dpc ();
	}

	scoped_spinlock_dpc (void) = delete;
	scoped_spinlock_dpc (scoped_spinlock_dpc &) = delete;
	scoped_spinlock_dpc (scoped_spinlock_dpc &&) = delete;
	scoped_spinlock_dpc &operator= (scoped_spinlock_dpc &&) = delete;
};
