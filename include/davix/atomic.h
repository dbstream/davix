/**
 * Functions for accessing memory locations atomically.
 * Copyright (C) 2024  dbstream
 *
 * We mostly just wrap the __atomic builtins in GCC, but we provide
 * shorthand definitions of common operations.
 */
#ifndef _DAVIX_ATOMIC_H
#define _DAVIX_ATOMIC_H 1

/* At a minimum, bools, ints, and longs must always be lock free. */
_Static_assert (__atomic_always_lock_free (sizeof(_Bool), 0), "bad atomics");
_Static_assert (__atomic_always_lock_free (sizeof(int), 0), "bad atomics");
_Static_assert (__atomic_always_lock_free (sizeof(long), 0), "bad atomics");

#define memory_order_relaxed __ATOMIC_RELAXED
#define memory_order_consume __ATOMIC_CONSUME
#define memory_order_acquire __ATOMIC_ACQUIRE
#define memory_order_release __ATOMIC_RELEASE
#define memory_order_acq_rel __ATOMIC_ACQ_REL
#define memory_order_seq_cst __ATOMIC_SEQ_CST

#define atomic_thread_fence(mo)		__atomic_thread_fence(mo)

#define atomic_load(p, mo)		__atomic_load_n ((p), (mo))
#define atomic_store(p, val, mo)	__atomic_store_n ((p), (val), (mo))

#define atomic_load_relaxed(p)		atomic_load ((p), memory_order_relaxed)
#define atomic_load_consume(p)		atomic_load ((p), memory_order_consume)
#define atomic_load_acquire(p)		atomic_load ((p), memory_order_acquire)

#define atomic_store_relaxed(p, val)	atomic_store ((p), (val), memory_order_relaxed)
#define atomic_store_release(p, val)	atomic_store ((p), (val), memory_order_release)

#define atomic_exchange(p, val, mo)	__atomic_exchange_n ((p), (val), (mo))

#define atomic_cmpxchg_weak(p, exp, val, mo, fail_mo)	\
	__atomic_compare_exchange_n ((p), (exp), (val), 1, (mo), (fail_mo))

#define atomic_cmpxchg(p, exp, val, mo, fail_mo)	\
	__atomic_compare_exchange_n ((p), (exp), (val), 0, (mo), (fail_mo))

#define atomic_cmpxchg_acqrel_acq(p, exp, val)		\
	atomic_cmpxchg((p), (exp), (val),		\
		memory_order_acq_rel, memory_order_acquire)

#define atomic_cmpxchg_relaxed(p, exp, val)		\
	atomic_cmpxchg((p), (exp), (val),		\
		memory_order_relaxed, memory_order_relaxed)

#define atomic_fetch_add(p, val, mo)	__atomic_fetch_add((p), (val), (mo))
#define atomic_fetch_sub(p, val, mo)	__atomic_fetch_sub((p), (val), (mo))
#define atomic_add_fetch(p, val, mo)	__atomic_add_fetch((p), (val), (mo))
#define atomic_sub_fetch(p, val, mo)	__atomic_sub_fetch((p), (val), (mo))

#define atomic_fetch_and(p, val, mo)	__atomic_fetch_and((p), (val), (mo))
#define atomic_fetch_or(p, val, mo)	__atomic_fetch_or((p), (val), (mo))
#define atomic_and_fetch(p, val, mo)	__atomic_and_fetch((p), (val), (mo))
#define atomic_or_fetch(p, val, mo)	__atomic_or_fetch((p), (val), (mo))

#define atomic_fetch_inc(p, mo)		atomic_fetch_add((p), 1, (mo))
#define atomic_fetch_dec(p, mo)		atomic_fetch_sub((p), 1, (mo))
#define atomic_inc_fetch(p, mo)		atomic_add_fetch((p), 1, (mo))
#define atomic_dec_fetch(p, mo)		atomic_sub_fetch((p), 1, (mo))

#define atomic_fetch_and_relaxed(p, val)	atomic_fetch_and((p), (val), memory_order_relaxed)
#define atomic_fetch_or_relaxed(p, val)		atomic_fetch_or((p), (val), memory_order_relaxed)
#define atomic_and_fetch_relaxed(p, val)	atomic_and_fetch((p), (val), memory_order_relaxed)
#define atomic_or_fetch_relaxed(p, val)		atomic_or_fetch((p), (val), memory_order_relaxed)

#define atomic_fetch_inc_relaxed(p)	atomic_fetch_inc((p), memory_order_relaxed)
#define atomic_fetch_dec_relaxed(p)	atomic_fetch_dec((p), memory_order_relaxed)
#define atomic_inc_fetch_relaxed(p)	atomic_inc_fetch((p), memory_order_relaxed)
#define atomic_dec_fetch_relaxed(p)	atomic_dec_fetch((p), memory_order_relaxed)

typedef _Bool atomic_flag_t;

#define atomic_test_and_set(p, mo)	__atomic_test_and_set((p), (mo))
#define atomic_clear(p, mo)		__atomic_clear((p), (mo))

#endif /* _DAVIX_ATOMIC_H */
