// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/spinlock.cc
 * KeRawSpinlock slowpaths.
 *
 * Copyright (C) 2025  dbstream
 */
#define SPINLOCK_DEBUG 1

#ifndef SPINLOCK_DEBUG
#define SPINLOCK_DEBUG 0
#endif

#include <Ke/raw_spinlock.h>
#include <davix/atomic.h>
#include <davix/export.h>

#if SPINLOCK_DEBUG
#include <Ke/context.h>
#include <davix/bug.h>
#endif

/**
 * KeRawSpinlock::lock_slowpath - spinlock acquire slowpath.
 * @lockval: last seen value in KeRawSpinlock::m_value
 */
void KeRawSpinlock::lock_slowpath(unsigned int lockval)
{
#if SPINLOCK_DEBUG
	/*
	 * It is a bug to acquire a spinlock in a preemptible context.
	 *
	 * NOTE: it is enough to disable one of {preemption, DPCs, IRQs} to
	 * effectively prevent preemption.  If all of them are enabled, invoke
	 * BUG().
	 */
	BUG_ON(KePreemptionEnabled() && KeDPCsEnabled() && KeIRQsEnabled());
#endif

	/*
	 * This is a simple TTAS lock.
	 */

	for (;;) {
		while (lockval == 1) {
			if (atomic_cmpxchg_weak_for_lock(&m_value, &lockval, 0))
				return;
		}

		do {
			smp_spinwait_hint();
			lockval = atomic_load_relaxed(&m_value);
		} while (lockval == 0U);
	}
}
EXPORT_SYMBOL(KeRawSpinlock::lock_slowpath)

