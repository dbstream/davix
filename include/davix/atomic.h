/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_ATOMIC_H
#define __DAVIX_ATOMIC_H

#include <davix/types.h>
#include <asm/atomic.h>

/*
 * Mainly used as an annotation in the source code. GCC __atomic builtins don't
 * play well with _Atomic for some reason.
 */
#define atomic

typedef enum memory_order {
	memory_order_relaxed = __ATOMIC_RELAXED,
	memory_order_consume = __ATOMIC_CONSUME,
	memory_order_acquire = __ATOMIC_ACQUIRE,
	memory_order_release = __ATOMIC_RELEASE,
	memory_order_acq_rel = __ATOMIC_ACQ_REL,
	memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order;

#define atomic_store __atomic_store_n
#define atomic_load __atomic_load_n
#define atomic_exchange __atomic_exchange_n

#define atomic_compare_exchange_strong(ptr, val, des, suc, fail) \
	__atomic_compare_exchange_n((ptr), (val), (des), 0, (suc), (fail))

#define atomic_compare_exchange_weak(ptr, val, des, suc, fail) \
	__atomic_compare_exchange_n((ptr), (val), (des), 1, (suc), (fail))

#define atomic_fetch_add __atomic_fetch_add
#define atomic_fetch_sub __atomic_fetch_sub
#define atomic_fetch_or __atomic_fetch_or
#define atomic_fetch_xor __atomic_fetch_xor
#define atomic_fetch_and __atomic_fetch_and

typedef atomic unsigned long refcnt_t;

static inline void grab(refcnt_t *refcnt)
{
	atomic_fetch_add(refcnt, 1, memory_order_relaxed);
}

static inline bool drop(refcnt_t *refcnt)
{
	return atomic_fetch_sub(refcnt, 1, memory_order_relaxed) == 1;
}

#endif /* __DAVIX_ATOMIC_H */
