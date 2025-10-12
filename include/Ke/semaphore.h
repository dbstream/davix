// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/semaphore.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Ke/time.h>
#include <OS/status.h>
#include <davix/atomic.h>
#include <davix/bug.h>

class KeSemaphore {
	static_assert(sizeof(unsigned int) == 4);
	static constexpr unsigned int SIGN_BIT = 1U << 31;

	unsigned int m_value = 0;

	void signal_slowpath(void);

	OSSTATUS wait_slowpath(
		unsigned int lv,
		bool interruptible,
		nsec_t timeout
	);
public:
	inline void reset(void)
	{
		BUG_ON(m_value & SIGN_BIT);
		m_value = 0;
	}

	inline void signal(void)
	{
		unsigned int value = atomic_inc_fetch(&m_value, _MO_Release);
		if (value & SIGN_BIT) {
			[[unlikely]];
			signal_slowpath();
		}
	}

	inline void wait(void)
	{
		unsigned int old = atomic_load_relaxed(&m_value);
		if (old & ~SIGN_BIT) {
			[[likely]];
			unsigned int des = old - 1;
			if (atomic_cmpxchg_weak_for_lock(&m_value, &old, des)) {
				[[likely]];
				return;
			}
		}

		wait_slowpath(old, false, NSEC_MAX);
	}

	inline OSSTATUS wait_timeout(nsec_t timeout)
	{
		unsigned int old = atomic_load_relaxed(&m_value);
		if (old & ~SIGN_BIT) {
			[[likely]];
			unsigned int des = old - 1;
			if (atomic_cmpxchg_weak_for_lock(&m_value, &old, des)) {
				[[likely]];
				return OS_STATUS_SUCCESS;
			}
		}

		return wait_slowpath(old, false, timeout);
	}

	inline OSSTATUS wait_interruptible(void)
	{
		unsigned int old = atomic_load_relaxed(&m_value);
		if (old & ~SIGN_BIT) {
			[[likely]];
			unsigned int des = old - 1;
			if (atomic_cmpxchg_weak_for_lock(&m_value, &old, des)) {
				[[likely]];
				return OS_STATUS_SUCCESS;
			}
		}

		return wait_slowpath(old, true, NSEC_MAX);
	}

	inline OSSTATUS wait_interruptible_timeout(nsec_t timeout)
	{
		unsigned int old = atomic_load_relaxed(&m_value);
		if (old & ~SIGN_BIT) {
			[[likely]];
			unsigned int des = old - 1;
			if (atomic_cmpxchg_weak_for_lock(&m_value, &old, des)) {
				[[likely]];
				return OS_STATUS_SUCCESS;
			}
		}

		return wait_slowpath(old, true, timeout);
	}

	inline OSSTATUS trywait(void)
	{
		unsigned int old = atomic_load_relaxed(&m_value);
		while (old & ~SIGN_BIT) {
			unsigned int des = old - 1;
			if (atomic_cmpxchg_weak_for_lock(&m_value, &old, des)) {
				[[likely]];
				return OS_STATUS_SUCCESS;
			}
		}

		[[unlikely]];
		return OS_STATUS_SEMA_WAIT_TIMED_OUT;
	}
};

