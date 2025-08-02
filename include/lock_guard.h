// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/lock_guard.h
 * RAII-based lock guard helper type.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

template<class LockType>
class scoped_lock_guard {
	LockType *m_lock;
	bool m_active;
public:
	scoped_lock_guard(scoped_lock_guard &) = delete;
	scoped_lock_guard(scoped_lock_guard &&) = delete;
	scoped_lock_guard &operator=(scoped_lock_guard &) = delete;
	scoped_lock_guard &operator=(scoped_lock_guard &&) = delete;

	inline scoped_lock_guard(LockType &lock)
		: m_lock(&lock), m_active(true)
	{
		m_lock->lock();
	}

	inline ~scoped_lock_guard(void)
	{
		if (m_active)
			m_lock->unlock();
	}

	inline void drop(void)
	{
		if (m_active) {
			m_active = false;
			m_lock->unlock();
		}
	}
};

