/**
 * Spinlock implementation
 * Copyright (C) 2024  dbstream
 *
 * A spinlock is a lightweight synchronization primitive. Spinlocks are most
 * suitable for bounded critical sections. In most cases, use mutexes instead
 * of spinlocks, as they sleep instead of spin-wait.
 */
#ifndef DAVIX_SPINLOCK_H
#define DAVIX_SPINLOCK_H 1

#include <davix/atomic.h>
#include <davix/stdbool.h>
#include <asm/irq.h>

/**
 * This is a very trivial spinlock implementation. There are two states:
 *   value=0  The lock is currently not held.
 *   value=1  The lock is currently held by someone.
 */
typedef struct spinlock {
	unsigned int value;
} spinlock_t;

/**
 * Initialize a dynamically-allocated spinlock.
 */
static inline void
spinlock_init (spinlock_t *lock)
{
	lock->value = 0;
}

/**
 * Unlock the spinlock.
 */
static inline void
__spin_unlock (spinlock_t *lock)
{
	atomic_store_release (&lock->value, 0);
}

/**
 * Optimistically try to acquire the lock with an atomic exchange. Returns true
 * for success.
 */
static inline bool
spin_lock_fastpath (spinlock_t *lock)
{
	return atomic_exchange (&lock->value, 1, memory_order_acquire) == 0;
}

/**
 * Slowpath for spinlock acquisition. Implements TTAS semantics.
 */
static inline void
spin_lock_slowpath (spinlock_t *lock)
{
	for (;;) {
		do
			arch_relax ();
		while (atomic_load_relaxed (&lock->value));

		if (spin_lock_fastpath (lock))
			break;
	}
}

/**
 * Try to acquire the spinlock. Returns true for success.
 */
static inline bool
spin_trylock (spinlock_t *lock)
{
	return spin_lock_fastpath (lock);
}

/**
 * Acquire the spinlock.
 */
static inline void
__spin_lock (spinlock_t *lock)
{
	if (spin_lock_fastpath (lock))
		return;

	spin_lock_slowpath (lock);
}

static inline bool
spin_lock_irq (spinlock_t *lock)
{
	bool flag = irq_save ();
	__spin_lock (lock);
	return flag;
}

static inline void
spin_unlock_irq (spinlock_t *lock, bool flag)
{
	__spin_unlock (lock);
	irq_restore (flag);
}

#endif /* DAVIX_SPINLOCK_H */
