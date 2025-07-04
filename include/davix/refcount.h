/**
 * Helper functions for managing reference counts.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <davix/atomic.h>

typedef unsigned long refcount_t;

/**
 * refcount_init - initialize a reference count.
 * @ref: pointer to reference count
 *
 * Initially, the reference count is 1.
 */
static inline void
refcount_init (refcount_t *ref)
{
	*ref = 1;
}

/**
 * refcount_inc - increment a reference count.
 * @ref: pointer to reference count
 */
static inline void
refcount_inc (refcount_t *ref)
{
	atomic_inc_fetch (ref, mo_relaxed);
}

/**
 * refcount_dec - decrement a reference count.
 * @ref: pointer to reference count
 * Returns true if the reference count reached zero, false if it did not reach
 * zero.
 */
static inline bool
refcount_dec (refcount_t *ref)
{
	return atomic_dec_fetch (ref, mo_acq_rel) == 0;
}

/**
 * refcount_inc_unless_zero - increment a reference count unless it is zero.
 * @ref: pointer to reference count
 * Returns true if we successfully incremented the reference count, false if we
 * ever observed zero.
 */
static inline bool
refcount_inc_unless_zero (refcount_t *ref)
{
	refcount_t prev = atomic_load_relaxed (ref);
	while (prev != 0) {
		bool ret = atomic_cmpxchg_weak (ref, &prev, prev + 1,
				mo_acquire, mo_relaxed);

		if (ret)
			return true;
	}

	return false;
}

