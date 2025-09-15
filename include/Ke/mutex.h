// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/mutex.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Ke/time.h>
#include <OS/status.h>
#include <davix/atomic.h>
#include <davix/bug.h>

class KeMutex {
	/*
	 * Lockval bits:
	 * Bit 0 - lock is held
	 * Bit 1 - lock has pending waiters
	 */
	unsigned char m_lockval = 0;

	OSSTATUS lock_slowpath(unsigned char lv, bool interrupt, nsec_t timeout);

	void unlock_slowpath(unsigned char lv);
public:
	inline void init(void)
	{
		m_lockval = 0;
	}

	inline void lock(void)
	{
		unsigned char expected = 0;
		if (!atomic_cmpxchg_weak_for_lock(&m_lockval, &expected, 1)) {
			[[unlikely]];
			OSSTATUS s = lock_slowpath(expected, false, NSEC_MAX);
			BUG_ON(!OS_SUCCESS(s));
		}
	}

	inline OSSTATUS lock_timeout(nsec_t timeout_ns)
	{
		unsigned char expected = 0;
		if (!atomic_cmpxchg_weak_for_lock(&m_lockval, &expected, 1)) {
			[[unlikely]];
			return lock_slowpath(expected, false, timeout_ns);
		}
		return OS_STATUS_SUCCESS;
	}

	inline OSSTATUS lock_interruptible(void)
	{
		unsigned char expected = 0;
		if (!atomic_cmpxchg_weak_for_lock(&m_lockval, &expected, 1)) {
			[[unlikely]];
			return lock_slowpath(expected, true, NSEC_MAX);
		}
		return OS_STATUS_SUCCESS;
	}

	inline OSSTATUS lock_interruptible_timeout(nsec_t timeout_ns)
	{
		unsigned char expected = 0;
		if (!atomic_cmpxchg_weak_for_lock(&m_lockval, &expected, 1)) {
			[[unlikely]];
			return lock_slowpath(expected, true, timeout_ns);
		}
		return OS_STATUS_SUCCESS;
	}

	inline OSSTATUS trylock(void)
	{
		unsigned char expected = 0;
		if (atomic_cmpxchg_for_lock(&m_lockval, &expected, 1)) {
			[[likely]];
			return OS_STATUS_SUCCESS;
		}

		return OS_STATUS_LOCK_ACQUIRE_TIMED_OUT;
	}

	inline void unlock(void)
	{
		unsigned char expected = 1;

		bool status = atomic_cmpxchg_weak(
			&m_lockval,
			&expected,
			0,
			_MO_Release,
			_MO_Relaxed
		);

		if (!status)
			unlock_slowpath(expected);
	}
};

