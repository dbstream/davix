/**
 * Semaphore implementation.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_SEMAPHORE_H
#define _DAVIX_SEMAPHORE_H 1

#include <davix/errno.h>
#include <davix/list.h>
#include <davix/spinlock.h>
#include <davix/time.h>

struct task;

struct semaphore {
	long count;
	spinlock_t lock;
	struct list waiters;
};

static inline void
sema_init (struct semaphore *sema)
{
	sema->count = 0;
	list_init (&sema->waiters);
}

/**
 * Unlock @sema.
 */
extern void
sema_unlock (struct semaphore *sema);

/**
 * Lock @sema.
 */
extern void
sema_lock (struct semaphore *sema);

/**
 * Lock @sema.  Interruptible verison.
 *
 * Returns an errno_t:
 *	ESUCCESS	The semaphore was successfully acquired.
 *	EINTR		The task received a signal.
 */
extern errno_t
sema_lock_interruptible (struct semaphore *sema);

/**
 * Lock @sema with a timeout of @us microseconds.  If @us is UINT64_MAX, no
 * timeout is used.
 *
 * Returns an errno_t:
 *	ESUCCESS	The semaphore was successfully acquired.
 *	ETIME		The semaphore timed out.
 */
extern errno_t
sema_lock_timeout (struct semaphore *sema, usecs_t us);

/**
 * Lock @sema with a timeout of @us microseconds.  If @us is UINT64_MAX, no
 * timeout is used.  Interruptible version.
 *
 * Returns an errno_t:
 *	ESUCCESS	The semaphore was successfully acquired.
 *	ETIME		The semaphore timed out.
 *	EINTR		The task received a signal.
 */
extern errno_t
sema_lock_timeout_interruptible (struct semaphore *sema, usecs_t us);

#endif /* _DAVIX_SEMAPHORE_H  */

