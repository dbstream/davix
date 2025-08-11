// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/raw_rwspinlock.h - Raw reader-writer spinlock primitive.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <davix/atomic.h>

static int KI_RAW_RWSPINLOCK_MAX_READERS = 0x40000000;

struct KeRawRWSpinlock {
	int m_value = 0;

	inline void init(void)
	{
		m_value = 0;
	}

	inline bool read_trylock(void)
	{
		int count = atomic_fetch_inc(&m_value, _MO_Acquire);
		if (count < 0) {
			[[unlikely]];
			atomic_fetch_dec(&m_value, _MO_Relaxed);
			return false;
		}

		return true;
	}

	inline bool write_trylock(void)
	{
		int count = 0;
		int newcount = -KI_RAW_RWSPINLOCK_MAX_READERS;
		if (atomic_cmpxchg_for_lock(&m_value, &count, newcount)) {
			[[likely]];
			return true;
		}

		return false;
	}

	inline void read_unlock(void)
	{
		atomic_fetch_dec(&m_value, _MO_Release);
	}

	inline void write_unlock(void)
	{
		atomic_fetch_add(&m_value, KI_RAW_RWSPINLOCK_MAX_READERS,
				_MO_Release);
	}

	inline void read_lock(void)
	{
		int count = atomic_fetch_inc(&m_value, _MO_Relaxed);
		while (count < 0) {
			smp_spinwait_hint();
			count = atomic_load_relaxed(&m_value);
		}
		/*
		 * This acquire fence is sequenced-after the above reads of
		 * m_value which we have observed as zero.  Therefore it
		 * synchronizes-with the atomic release operation in
		 * write_unlock (or the lock was never held by a writer).
		 */
		atomic_thread_fence(_MO_Acquire);
	}

	inline void write_lock(void)
	{
		int count = 0;
		int newcount = -KI_RAW_RWSPINLOCK_MAX_READERS;
		for (;;) {
			if (atomic_cmpxchg_weak_for_lock(&m_value, &count, newcount)) {
				[[likely]];
				return;
			}
			while (count != 0) {
				smp_spinwait_hint();
				count = atomic_load_relaxed(&m_value);
			}
		}
	}

	inline void downgrade(void)
	{
		atomic_fetch_add(&m_value, KI_RAW_RWSPINLOCK_MAX_READERS + 1,
				_MO_Release);
	}
};

