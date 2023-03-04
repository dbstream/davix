/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_SPINLOCK_H
#define __DAVIX_SPINLOCK_H

#include <davix/atomic.h>
#include <davix/entry.h>
#include <davix/types.h>
#include <asm/irq.h>

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

static inline void _spin_acquire(spinlock_t *lock)
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

static inline void _spin_release(spinlock_t *lock)
{
	atomic_store(&lock->contended, 0, memory_order_release);
}

static inline void spin_acquire(spinlock_t *lock) acquires(lock)
{
	preempt_disable();
	_spin_acquire(lock);
}

static inline void spin_release(spinlock_t *lock) releases(lock)
{
	_spin_release(lock);
	preempt_enable();
}

/*
 * Return value: interrupt enable flag to be passed to spin_release_irq().
 */
static inline warn_unused int spin_acquire_irq(spinlock_t *lock) acquires(lock)
{
	int ret = interrupts_enabled();
	disable_interrupts();
	_spin_acquire(lock);
	return ret;
}

/*
 * Pass the interrupt enable flag from spin_acquire_irq().
 */
static inline void spin_release_irq(spinlock_t *lock, int flag) releases(lock)
{
	_spin_release(lock);
	if(flag)
		enable_interrupts();	
}

#endif /* __DAVIX_SPINLOCK_H */
