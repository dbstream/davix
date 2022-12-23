/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_SPINLOCK_H
#define __DAVIX_SPINLOCK_H

#include <davix/atomic.h>

typedef struct spinlock {
	atomic int contended;
} spinlock_t;

#define SPINLOCK_INIT(name) ((struct spinlock) { 0 })

static inline void spinlock_init(spinlock_t *lock)
{
	lock->contended = 0;
}

/*
 * Try to acquire a spinlock. Return non-zero to indicate success.
 */
static inline int spin_try_acquire(spinlock_t *lock)
{
	int expected = 0;
	return atomic_compare_exchange_strong(&lock->contended, &expected,
		1, memory_order_acquire, memory_order_relaxed);
}

static inline void spin_acquire(spinlock_t *lock)
{
	for(;;) {
		int expected = 0;
		if(atomic_compare_exchange_weak(&lock->contended, &expected,
			1, memory_order_acquire, memory_order_relaxed))
				return;
		do {
			relax();
		} while(atomic_load(&lock->contended,
			memory_order_relaxed) != 0);
	}
}

static inline void spin_release(spinlock_t *lock)
{
	atomic_store(&lock->contended, 0, memory_order_release);
}

#endif /* __DAVIX_SPINLOCK_H */
