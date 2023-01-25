/* SPDX-License-Identifer: MIT */
#ifndef __DAVIX_SEMAPHORE_H
#define __DAVIX_SEMAPHORE_H

#include <davix/spinlock.h>

struct semaphore {
	spinlock_t lock;
	unsigned count;
};

#define SEMAPHORE_INIT(name, count) { SPINLOCK_INIT(name.lock), count }

static inline void semaphore_init(struct semaphore *sem, unsigned long count)
{
	spinlock_init(&sem->lock);
	sem->count = count;
}

/*
 * Try to acquire the semaphore immediately (without waiting).
 * Return value: was the semaphore successfully acquired?
 */
int sem_try_wait(struct semaphore *sem);

/*
 * Wait for a semaphore.
 */
void sem_wait(struct semaphore *sem);

/*
 * Signal a semaphore.
 */
void sem_signal(struct semaphore *sem);

#endif /* __DAVIX_SEMAPHORE_H */
