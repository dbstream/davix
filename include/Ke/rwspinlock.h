// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/rwspinlock.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Ke/context.h>
#include <Ke/raw_rwspinlock.h>

class KeRWSpinlock {
	KeRawRWSpinlock m_lock;
public:
	inline void init(void)
	{
		m_lock.init();
	}

	inline bool read_trylock(void)
	{
		KeDisablePreemption();
		if (m_lock.read_trylock())
			return true;
		KeEnablePreemption();
		return false;
	}

	inline void read_lock(void)
	{
		KeDisablePreemption();
		m_lock.read_lock();
	}

	inline void read_unlock(void)
	{
		m_lock.read_unlock();
		KeEnablePreemption();
	}

	inline bool write_trylock(void)
	{
		KeDisablePreemption();
		if (m_lock.write_trylock())
			return true;
		KeEnablePreemption();
		return false;
	}

	inline void write_lock(void)
	{
		KeDisablePreemption();
		m_lock.write_lock();
	}

	inline void write_unlock(void)
	{
		m_lock.write_unlock();
		KeEnablePreemption();
	}

	inline void downgrade(void)
	{
		m_lock.downgrade();
	}
};

class KeDPCRWSpinlock {
	KeRawRWSpinlock m_lock;
public:
	inline void init(void)
	{
		m_lock.init();
	}

	inline bool read_trylock(void)
	{
		KeDisableDPCs();
		if (m_lock.read_trylock())
			return true;
		KeEnableDPCs();
		return false;
	}

	inline void read_lock(void)
	{
		KeDisableDPCs();
		m_lock.read_lock();
	}

	inline void read_unlock(void)
	{
		m_lock.read_unlock();
		KeEnableDPCs();
	}

	inline bool write_trylock(void)
	{
		KeDisableDPCs();
		if (m_lock.write_trylock())
			return true;
		KeEnableDPCs();
		return false;
	}

	inline void write_lock(void)
	{
		KeDisableDPCs();
		m_lock.write_lock();
	}

	inline void write_unlock(void)
	{
		m_lock.write_unlock();
		KeEnableDPCs();
	}

	inline void downgrade(void)
	{
		m_lock.downgrade();
	}
};

