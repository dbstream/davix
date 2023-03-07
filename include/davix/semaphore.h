/* SPDX-License-Identifer: MIT */
#ifndef __DAVIX_SEMAPHORE_H
#define __DAVIX_SEMAPHORE_H

#include <davix/types.h>
#include <davix/time.h>
#include <davix/spinlock.h>
#include <davix/list.h>

struct task;

struct semaphore {
	struct list waiters;
	spinlock_t lock;
	unsigned count;
};

struct semaphore_waiter {
	struct list list;
	struct task *task;
	bool flag;
};

#define SEMAPHORE_INIT(name, count) { LIST_INIT(name.waiters), SPINLOCK_INIT(name.lock), count }

static inline void semaphore_init(struct semaphore *sem, unsigned long count)
{
	list_init(&sem->waiters);
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
 * Wait for a semaphore, with a timeout.
 * Returns zero or ETIME.
 */
int sem_wait_timeout(struct semaphore *sem, nsecs_t timeout);

/*
 * Signal a semaphore.
 */
void sem_signal(struct semaphore *sem);

#endif /* __DAVIX_SEMAPHORE_H */
