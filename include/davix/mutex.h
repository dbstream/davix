/**
 * Blocking mutex implementation with support for timeouts.
 * Copyright (C) 2024  dbstream
 *
 * Locking rules:
 * - Mutexes don't disable preemption. However it is fully okay to do so
 *   yourself at any time. Beware that mutex_lock() can still go to sleep.
 * - The same task that holds a lock must unlock it.
 */
#ifndef __DAVIX_MUTEX_H
#define __DAVIX_MUTEX_H 1

#include <davix/errno.h>
#include <davix/list.h>
#include <davix/spinlock.h>
#include <davix/time.h>

struct task;

/**
 * A mutex contains three things:
 * - Pointer to current owner (or NULL when unlocked).
 * - Spinlock for synchronization.
 * - A linked list of mutex waiters.
 *
 * However, for fast acquire and release when the mutex is uncontended, we use
 * the two lower bits of the owner pointer as storage for the spinlock, and a
 * 'handoff' flag, which indicates that there are additional mutex waiters.
 *
 * owner_and_lock:
 *	NULL	0b00	the mutex is unlocked
 *	task	0b00	the mutex is locked and the waitlist is unlocked
 *	task	0b10	the mutex is locked and someone is waiting
 *	any	0bX1	the mutex is contended
 */
struct mutex {
	unsigned long owner_and_lock;
	struct list waiters;
};

static inline void
mutex_init (struct mutex *mutex)
{
	mutex->owner_and_lock = 0;
	list_init (&mutex->waiters);
}

/**
 * Unlock @mutex, which is held by the currently executing task.
 */
extern void
mutex_unlock (struct mutex *mutex);

/**
 * Lock @mutex.
 */
extern void
mutex_lock (struct mutex *mutex);

/**
 * Lock @mutex. Interruptible version.
 *
 * Returns an errno_t:
 *	ESUCCESS	The lock was successfully acquired.
 *	EINTR		The task received a pending signal.
 */
extern errno_t
mutex_lock_interruptible (struct mutex *mutex);

/**
 * Lock @mutex with a timeout of @us microseconds. If @us is UINT64_MAX, no
 * timeout is used.
 *
 * Returns an errno_t:
 *	ESUCCESS	The lock was successfully acquired.
 *	ETIME		The lock timed out.
 */
extern errno_t
mutex_lock_timeout (struct mutex *mutex, usecs_t us);

/**
 * Lock @mutex with a timeout of @us microseconds. If @us is UINT64_MAX, no
 * timeout is used. Interruptible version.
 *
 * Returns an errno_t:
 *	ESUCCESS	The lock was successfully acquired.
 *	ETIME		The lock timed out.
 *	EINTR		The task received a pending signal.
 */
extern errno_t
mutex_lock_timeout_interruptible (struct mutex *mutex, usecs_t us);

#endif /* __DAVIX_MUTEX_H */
