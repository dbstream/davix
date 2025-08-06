// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/raw_spinlock.h
 * KeRawSpinlock
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <davix/atomic.h>

struct KeRawSpinlock {
	unsigned int m_value = 1;

	void lock_slowpath(unsigned int value);

	inline void init(void)
	{
		m_value = 1;
	}

	/**
	 * KeRawSpinlock::trylock - try to acquire the lock without spinning.
	 */
	inline bool trylock(void)
	{
		unsigned int exp = 1;
		/*
		 * NOTE: on some architectures, non-weak cmpxchg is implemented
		 * as a ll/sc loop, which arguably counts as spinning.
		 */
		return atomic_cmpxchg_for_lock(&m_value, &exp, 0);
	}

	/**
	 * KeRawSpinlock::lock - spin and acquire the lock.
	 */
	inline void lock(void)
	{
		unsigned int exp = 1;
		if (!atomic_cmpxchg_weak_for_lock(&m_value, &exp, 0)) {
			[[unlikely]];
			lock_slowpath(exp);
		}
	}

	/**
	 * KeRawSpinlock::unlock - release the lock.
	 */
	inline void unlock(void)
	{
		atomic_store_release(&m_value, 1);
	}

	/**
	 * KeRawSpinlock::locked - test if the spinlock is held by anyone.
	 *
	 * NOTE: this function does not provide any stricter memory ordering
	 * than relaxed. Callers can use trylock() or smp_mb() before and/or
	 * after invoking this function if they demand strong memory ordering.
	 */
	inline bool locked(void)
	{
		return atomic_load_relaxed(&m_value) != 1;
	}
};

