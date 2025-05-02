/**
 * Atomics and memory barriers.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <asm/atomic.h>

static_assert (__atomic_always_lock_free (sizeof (bool), 0));
static_assert (__atomic_always_lock_free (sizeof (char), 0));
static_assert (__atomic_always_lock_free (sizeof (short), 0));
static_assert (__atomic_always_lock_free (sizeof (int), 0));
static_assert (__atomic_always_lock_free (sizeof (long), 0));
static_assert (__atomic_always_lock_free (sizeof (void *), 0));

enum {
	mo_relaxed	= __ATOMIC_RELAXED,	// relaxed ordering
	mo_consume	= __ATOMIC_CONSUME,	// consume ordering
	mo_acquire	= __ATOMIC_ACQUIRE,	// acquire ordering
	mo_release	= __ATOMIC_RELEASE,	// release ordering
	mo_acq_rel	= __ATOMIC_ACQ_REL,	// acquire-release ordering
	mo_seq_cst	= __ATOMIC_SEQ_CST	// sequentially consistent ordering
};

#define atomic_thread_fence(mo) __atomic_thread_fence(mo)

#define atomic_load(p, mo) __atomic_load_n((p), (mo))
#define atomic_store(p, val, mo) __atomic_store_n((p), (val), (mo))

#define atomic_load_relaxed(p) atomic_load((p), mo_relaxed)
#define atomic_load_acquire(p) atomic_load((p), mo_acquire)

#define atomic_store_relaxed(p, val) atomic_store((p), (val), mo_relaxed)
#define atomic_store_release(p, val) atomic_store((p), (val), mo_release)

#define atomic_exchange(p, val, mo) __atomic_exchange_n((p), (val), (mo))

#define atomic_exchange_acquire(p, val) atomic_exchange((p), (val), mo_acquire)
#define atomic_exchange_acq_rel(p, val) atomic_exchange((p), (val), mo_acq_rel)

#define atomic_cmpxchg_weak(p, exp, des, success_mo, fail_mo)		\
	__atomic_compare_exchange_n((p), (exp), (des), 1, (success_mo), (fail_mo))

#define atomic_cmpxchg(p, exp, des, success_mo, fail_mo)		\
	__atomic_compare_exchange_n((p), (exp), (des), 0, (success_mo), (fail_mo))

#define atomic_cmpxchg_for_lock(p, exp, des)				\
	atomic_cmpxchg((p), (exp), (des), mo_acquire, mo_relaxed)

#define atomic_fetch_add(p, val, mo) __atomic_fetch_add((p), (val), (mo))
#define atomic_fetch_sub(p, val, mo) __atomic_fetch_sub((p), (val), (mo))
#define atomic_add_fetch(p, val, mo) __atomic_add_fetch((p), (val), (mo))
#define atomic_sub_fetch(p, val, mo) __atomic_sub_fetch((p), (val), (mo))

#define atomic_fetch_inc(p, mo) atomic_fetch_add((p), 1, (mo))
#define atomic_fetch_dec(p, mo) atomic_fetch_sub((p), 1, (mo))
#define atomic_inc_fetch(p, mo) atomic_add_fetch((p), 1, (mo))
#define atomic_dec_fetch(p, mo) atomic_sub_fetch((p), 1, (mo))

#ifdef smp_mb
#ifndef __arch_provides_smp_mb
#define __arch_provides_smp_mb
#endif
#endif
#ifdef smp_wmb
#ifndef __arch_provides_smp_wmb
#define __arch_provides_smp_wmb
#endif
#endif
#ifdef smp_rmb
#ifndef __arch_provides_smb_rmb
#define __arch_provides_smp_rmb
#endif
#endif
#ifdef smp_spinlock_hint
#ifndef __arch_provides_smp_spinlock_hint
#define __arch_provides_smp_spinlock_hint
#endif
#endif

#ifndef __arch_provides_smp_mb
static inline void smp_mb (void) { asm volatile ("" ::: "memory" ); }
#endif

#ifndef __arch_provides_smp_wmb
static inline void smb_wmb (void) { asm volatile ("" ::: "memory"); }
#endif

#ifndef __arch_provides_smp_rmb
static inline void smp_rmb (void) { asm volatile ("" ::: "memory"); }
#endif

#ifndef __arch_provides_smp_spinlock_hint
static inline void smp_spinlock_hint (void) { asm volatile ("" ::: "memory" ); }
#endif
