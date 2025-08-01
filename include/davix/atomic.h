// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/davix/atomic.h
 * Atomic operations and memory barriers.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <asm/atomic.h>

/**
 * barrier - compiler optimization barrier.
 */
#ifndef barrier
#define barrier() do { asm volatile("" ::: "memory"); } while (0)
#endif

/**
 * smp_rmb - partial load ordering memory barrier.
 *
 * A read memory barrier provides the guarantee that all load operations that
 * occur before the barrier in program order will be globally visible to CPUs
 * before all load operations that occur after the barrier.
 *
 * smp_rmb() is not required to have any effect on stores.
 */
#ifndef smp_rmb
#define smp_rmb() barrier()
#endif

/**
 * smp_wmb - partial store ordering memory barrier.
 *
 * A write memory barrier provides the guarantee that all store operations that
 * occur before the barrier in program order will be globally visible to CPUs
 * before all store operations that occur after the barrier.
 *
 * smp_wmb() is not required to have any effect on loads.
 */
#ifndef smp_wmb
#define smp_wmb() barrier()
#endif

/**
 * smp_mb - partial ordering memory barrier for both loads and stores.
 *
 * A general memory barrier provides the guarantee that all memory operations
 * that occur before the barrier in program order will be globally visible to
 * CPUs before all memory operations that occur after the barrier.
 *
 * smp_mb() can be substituted for either of smp_rmb() and smp_wmb().
 */
#ifndef smp_mb
#define smp_mb() barrier()
#endif

/**
 * smp_spinwait_hint - spinwait loop CPU hint.
 */
#ifndef smp_spinwait_hint
#define smp_spinwait_hint() barrier()
#endif

/*
 * atomic_* memory orderings.
 */
enum {
	_MO_Relaxed = __ATOMIC_RELAXED,
	_MO_Consume = __ATOMIC_CONSUME,
	_MO_Acquire = __ATOMIC_ACQUIRE,
	_MO_Release = __ATOMIC_RELEASE,
	_MO_AcqRel = __ATOMIC_ACQ_REL,
	_MO_SeqCst = __ATOMIC_SEQ_CST,
};

#define atomic_thread_fence(mo) __atomic_thread_fence(mo)

#define atomic_load(p, mo) __atomic_load_n((p), (mo))
#define atomic_store(p, val, mo) __atomic_store_n((p), (val), (mo))

#define atomic_load_relaxed(p) atomic_load((p), _MO_Relaxed)
#define atomic_load_acquire(p) atomic_load((p), _MO_Acquire)

#define atomic_store_relaxed(p, val) atomic_store((p), (val), _MO_Relaxed)
#define atomic_store_release(p, val) atomic_store((p), (val), _MO_Release)

#define atomic_exchange(p, val, mo) __atomic_exchange_n((p), (val), (mo))

#define atomic_exchange_acquire(p, val) atomic_exchange((p), (val), _MO_Acquire)
#define atomic_exchange_acq_rel(p, val) atomic_exchange((p), (val), _MO_AcqRel)

#define atomic_cmpxchg_weak(p, exp, des, success_mo, fail_mo)           \
        __atomic_compare_exchange_n((p), (exp), (des), 1, (success_mo), (fail_mo))

#define atomic_cmpxchg(p, exp, des, success_mo, fail_mo)                \
        __atomic_compare_exchange_n((p), (exp), (des), 0, (success_mo), (fail_mo))

#define atomic_cmpxchg_for_lock(p, exp, des)                            \
        atomic_cmpxchg((p), (exp), (des), _MO_Acquire, _MO_Relaxed)

#define atomic_cmpxchg_weak_for_lock(p, exp, des)			\
	atomic_cmpxchg_weak((p), (exp), (des), _MO_Acquire, _MO_Relaxed)

#define atomic_fetch_add(p, val, mo) __atomic_fetch_add((p), (val), (mo))
#define atomic_fetch_sub(p, val, mo) __atomic_fetch_sub((p), (val), (mo))
#define atomic_add_fetch(p, val, mo) __atomic_add_fetch((p), (val), (mo))
#define atomic_sub_fetch(p, val, mo) __atomic_sub_fetch((p), (val), (mo))

#define atomic_fetch_inc(p, mo) atomic_fetch_add((p), 1, (mo))
#define atomic_fetch_dec(p, mo) atomic_fetch_sub((p), 1, (mo))
#define atomic_inc_fetch(p, mo) atomic_add_fetch((p), 1, (mo))
#define atomic_dec_fetch(p, mo) atomic_sub_fetch((p), 1, (mo))

#define atomic_fetch_and(p, val, mo) __atomic_fetch_and((p), (val), (mo))
#define atomic_fetch_or(p, val, mo) __atomic_fetch_or((p), (val), (mo))

#define atomic_and_fetch(p, val, mo) __atomic_and_fetch((p), (val), (mo))
#define atomic_or_fetch(p, val, mo) __atomic_or_fetch((p), (val), (mo))

