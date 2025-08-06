// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/spinlock.h
 * Wrapper types for KeRawSpinlock.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Ke/context.h>
#include <Ke/raw_spinlock.h>

class KeSpinlock {
	KeRawSpinlock m_lock;
public:
	inline void init(void)
	{
		m_lock.init();
	}

	inline bool trylock(void)
	{
		KeDisablePreemption();
		if (m_lock.trylock())
			return true;
		KeEnablePreemption();
		return false;
	}

	inline void lock(void)
	{
		KeDisablePreemption();
		m_lock.lock();
	}

	inline void unlock(void)
	{
		m_lock.unlock();
		KeEnablePreemption();
	}

	inline bool locked(void)
	{
		return m_lock.locked();
	}
};

class KeDPCSpinlock {
	KeRawSpinlock m_lock;
public:
	inline void init(void)
	{
		m_lock.init();
	}

	inline bool trylock(void)
	{
		KeDisableDPCs();
		if (m_lock.trylock())
			return true;
		KeEnableDPCs();
		return false;
	}

	inline void lock(void)
	{
		KeDisableDPCs();
		m_lock.lock();
	}

	inline void unlock(void)
	{
		m_lock.unlock();
		KeEnableDPCs();
	}

	inline bool locked(void)
	{
		return m_lock.locked();
	}
};

class KeIRQSpinlock {
	KeRawSpinlock m_lock;
public:
	inline void init(void)
	{
		m_lock.init();
	}

	inline bool trylock(void)
	{
		KeDisableIRQs();
		if (m_lock.trylock())
			return true;
		KeEnableIRQs();
		return false;
	}

	inline void lock(void)
	{
		KeDisableIRQs();
		m_lock.lock();
	}

	inline void unlock(void)
	{
		m_lock.unlock();
		KeEnableIRQs();
	}

	inline bool locked(void)
	{
		return m_lock.locked();
	}
};

