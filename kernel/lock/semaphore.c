/**
 * Semaphore implementation with support for timeouts.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/atomic.h>
#include <davix/context.h>
#include <davix/panic.h>
#include <davix/sched.h>
#include <davix/semaphore.h>
#include <davix/stdint.h>
#include <davix/timer.h>
#include <asm/irq.h>

static inline bool
sema_unlock_fastpath (struct semaphore *sema)
{
	spin_lock (&sema->lock);
	if (!list_empty (&sema->waiters))
		return false;

	sema->count++;
	spin_unlock (&sema->lock);
	return true;
}

static inline bool
sema_lock_fastpath (struct semaphore *sema)
{
	spin_lock (&sema->lock);
	long count = sema->count - 1;
	if (count < 0)
		return false;

	sema->count = count;
	spin_unlock (&sema->lock);
	return true;
}

static inline bool
has_pending_signal (unsigned int sleep_state)
{
	(void) sleep_state;
	return false;
}

struct sema_waiter {
	struct ktimer timer;
	struct list list;
	struct task *task;
	bool got_lock;
};

static void
wake_up_waiter (struct sema_waiter *w)
{
	sched_wake (w->task);
}

/**
 * Unlock @sema.
 */
void
sema_unlock (struct semaphore *sema)
{
	if (sema_unlock_fastpath (sema))
		return;

	struct sema_waiter *w = list_item (struct sema_waiter,
			list, sema->waiters.next);
	w->got_lock = true;
	list_delete (&w->list);
	wake_up_waiter (w);
	spin_unlock (&sema->lock);
}

static void
sema_waiter_timeout (struct ktimer *timer)
{
	struct sema_waiter *w = struct_parent (struct sema_waiter,
			timer, timer);

	wake_up_waiter (w);
}

static errno_t
sema_lock_common (struct semaphore *sema,
		usecs_t timeout, unsigned int sleep_state)
{
	if (!timeout) {
		spin_unlock (&sema->lock);
		return EAGAIN;
	}

	if (timeout != UINT64_MAX)
		timeout += us_since_boot ();

	struct task *me = get_current_task ();

	struct sema_waiter w;
	w.timer.callback = sema_waiter_timeout;
	w.timer.expiry = timeout;
	w.timer.triggered = true;
	w.task = me;
	w.got_lock = false;

	irq_disable ();
	if (has_pending_signal (sleep_state)) {
		irq_enable ();
		spin_unlock (&sema->lock);
		return EINTR;
	}

	if (timeout != UINT64_MAX)
		ktimer_add (&w.timer);

	me->state = sleep_state;
	list_insert_back (&sema->waiters, &w.list);
	__spin_unlock (&sema->lock);
	schedule ();
	
	errno_t e = EINTR;
	if (timeout != UINT64_MAX && ktimer_remove (&w.timer))
		e = ETIME;

	__spin_lock (&sema->lock);
	if (w.got_lock)
		e = ESUCCESS;
	else
		list_delete (&w.list);

	irq_enable ();
	spin_unlock (&sema->lock);
	return e;
}

/**
 * Lock @sema.
 */
void
sema_lock (struct semaphore *sema)
{
	if (sema_lock_fastpath (sema))
		return;

	errno_t e;
	e = sema_lock_common (sema, UINT64_MAX, TASK_UNINTERRUPTIBLE);
	if (e != ESUCCESS)
		panic ("sema_lock (infallible variant): acquisition failed");
}

/**
 * Lock @sema.  Interruptible verison.
 *
 * Returns an errno_t:
 *	ESUCCESS	The semaphore was successfully acquired.
 *	EINTR		The task received a signal.
 */
errno_t
sema_lock_interruptible (struct semaphore *sema)
{
	if (sema_lock_fastpath (sema))
		return ESUCCESS;

	return sema_lock_common (sema, UINT64_MAX, TASK_INTERRUPTIBLE);
}

/**
 * Lock @sema with a timeout of @us microseconds.  If @us is UINT64_MAX, no
 * timeout is used.
 *
 * Returns an errno_t:
 *	ESUCCESS	The semaphore was successfully acquired.
 *	ETIME		The semaphore timed out.
 */
errno_t
sema_lock_timeout (struct semaphore *sema, usecs_t us)
{
	if (sema_lock_fastpath (sema))
		return ESUCCESS;

	return sema_lock_common (sema, us, TASK_UNINTERRUPTIBLE);
}

/**
 * Lock @sema with a timeout of @us microseconds.  If @us is UINT64_MAX, no
 * timeout is used.  Interruptible version.
 *
 * Returns an errno_t:
 *	ESUCCESS	The semaphore was successfully acquired.
 *	ETIME		The semaphore timed out.
 *	EINTR		The task received a signal.
 */
errno_t
sema_lock_timeout_interruptible (struct semaphore *sema, usecs_t us)
{
	if (sema_lock_fastpath (sema))
		return ESUCCESS;

	return sema_lock_common (sema, us, TASK_INTERRUPTIBLE);
}

