/**
 * Safe atomic reference counting.
 * Copyright (C) 2024  dbstream
 */
#ifndef __DAVIX_REFCOUNT_H
#define __DAVIX_REFCOUNT_H 1

#include <davix/atomic.h>

typedef unsigned long refcount_t;

/**
 * Set the reference count to a particular value.
 * NOTE: this uses relaxed ordering and should only be done during
 * initialization of a refcount_t.
 */
static inline void
refcount_set (refcount_t *ref, refcount_t value)
{
	atomic_store (ref, value, memory_order_relaxed);
}

/**
 * Increment @ref by one.
 */
static inline void
refcount_inc (refcount_t *ref)
{
	atomic_inc_fetch (ref, memory_order_relaxed);
}

/**
 * Increment @ref by @amount.
 */
static inline void
refcount_add (refcount_t *ref, refcount_t amount)
{
	atomic_add_fetch (ref, amount, memory_order_relaxed);
}

/** 
 * Decrement @ref by one. Returns true if the reference count reached zero.
 */
static inline int
refcount_dec (refcount_t *ref)
{
	if (!atomic_dec_fetch (ref, memory_order_release)) {
		atomic_thread_fence (memory_order_acquire);
		return 1;
	}

	return 0;
}

/**
 * Decrement @ref by @amount. Returns true if the reference count reached zero.
 */
static inline int
refcount_sub (refcount_t *ref, refcount_t amount)
{
	if (!atomic_sub_fetch (ref, amount, memory_order_acq_rel)) {
		atomic_thread_fence (memory_order_acquire);
		return 1;
	}

	return 0;
}

#endif /* __DAVIX_REFCOUNT  */