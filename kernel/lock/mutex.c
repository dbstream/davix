/**
 * Blocking mutex implementation with support for timeouts.
 * Copyright (C) 2024  dbstream
 *
 * Locking rules:
 * - Mutexes don't disable preemption. However it is fully okay to do so
 *   yourself at any time. Beware that mutex_lock() can still go to sleep.
 * - The same task that holds a lock must unlock it.
 */
#include <davix/atomic.h>
#include <davix/mutex.h>
#include <davix/panic.h>
#include <davix/sched.h>
#include <davix/timer.h>

#include <davix/printk.h>

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
 * This means, in the uncontended case:
 * - Mutex acquire is one cmpxchg.
 * - Mutex release is one cmpxchg.
 *
 * owner_and_lock:
 *	NULL	0b00	the mutex is unlocked
 *	task	0b00	the mutex is locked and the waitlist is unlocked
 *	task	0b10	the mutex is locked and someone is waiting
 *	any	0b01	the mutex is contended
 *	any	0b11	the mutex is contended and a pending wakeup is available
 */

#define CONTENDED 1UL
#define HAS_WAITER 2UL

#define CONTENTION_FLAGS (CONTENDED | HAS_WAITER)

/**
 * Attempt fast mutex lock. Returns true on success, otherwise writes the value
 * found in @mutex->owner_and_lock into *@expected.
 */
static inline bool
mutex_lock_fastpath (struct mutex *mutex, struct task *self, unsigned long *expected)
{
	*expected = 0;
	unsigned long desired = (unsigned long) self;

	return atomic_cmpxchg_acqrel_acq (&mutex->owner_and_lock, expected, desired);
}

/**
 * Attempt fast mutex unlock. Returns true on success, otherwise writes the
 * value found in @mutex->owner_and_lock into *@expected.
 */
static inline bool
mutex_unlock_fastpath (struct mutex *mutex, struct task *self, unsigned long *expected)
{
	*expected = (unsigned long) self;
	unsigned long desired = 0;

	return atomic_cmpxchg_acqrel_acq (&mutex->owner_and_lock, expected, desired);
}


/**
 * Ensure that the task that acquired a mutex is also that which releases it.
 */
static inline void
check_expected_owner (struct mutex *mtx, struct task *self, unsigned long owner)
{
	if ((owner & ~CONTENTION_FLAGS) != (unsigned long) self)
		panic ("mutex_unlock(%p) was acquired and released on different tasks (acquired by %s, released by %s)",
			mtx, ((struct task *) (owner & ~CONTENTION_FLAGS))->comm, self->comm);
}

/**
 * Acquire the mutex contention flag. We currently own the mutex.
 *
 * This function returns true if wake_up_waiter_and_disown must be called.
 */
static bool
acquire_contention_flag_for_unlock (struct mutex *mutex, unsigned long expected)
{
	unsigned long desired;
	for (;;) {
		/**
		 * If the lock is contended by someone else, we want to set
		 * HAS_WAITER. Otherwise, go for CONTENDED.
		 */
		if (expected & CONTENDED)
			desired = expected | HAS_WAITER;
		else
			desired = (expected & ~HAS_WAITER) | CONTENDED;

		/**
		 * If we set HAS_WAITER, we no longer have to do anything. The
		 * contending task will wake up a waiter.
		 */
		if (atomic_cmpxchg_acqrel_acq (&mutex->owner_and_lock, &expected, desired))
			return !(desired & HAS_WAITER);

		/** 
		 * If all waiters have left, try to fast-unlock.
		 */
		if (!(expected & CONTENTION_FLAGS)) {
			desired = 0;
			if (atomic_cmpxchg_acqrel_acq (&mutex->owner_and_lock, &expected, desired))
				return false;
		}

		arch_relax ();
	}
}

/**
 * Acquire the mutex contention flag for locking the mutex.
 */
static void
acquire_contention_flag_for_lock (struct mutex *mutex, unsigned long *expected)
{
	unsigned long desired;
	for (;;) {
		/**
		 * Avoid unnecessarily spinning on cmpxchg.
		 */
		if (*expected & CONTENDED) {
			arch_relax ();
			*expected = atomic_load_relaxed (&mutex->owner_and_lock);
			continue;
		}

		desired = (*expected & ~HAS_WAITER) | CONTENDED;
		if (atomic_cmpxchg_acqrel_acq (&mutex->owner_and_lock, expected, desired)) {
			*expected = desired;
			return;
		}
	}
}

/**
 * Mutex waiter structure.
 * Contains:
 * - Linked list entry.
 * - Pointer to the task that is sleeping.
 *
 * task is cleared to NULL when a waiter is awaken.
 */
struct mutex_waiter {
	struct list list;
	struct task *task;
};

/**
 * Mutex waiter with timeout structure.
 */
struct mutex_timeout_waiter {
	struct ktimer timer;
	struct mutex_waiter waiter;
};

/**
 * Wake up @task.
 */
static void
wake_up_task (struct task *task)
{
	sched_wake (task);
}

/**
 * Check if the current task has any pending signal. (stub)
 */
static inline bool
has_pending_signal (unsigned int task_state)
{
	(void) task_state;
	return false;
}

/**
 * If IRQs are disabled when mutex_lock() is called, the following deadlock can
 * occur:
 *    CPU0                                        CPU1
 *  mutex_lock()
 *  ...
 *  mutex_unlock()                              mutex_lock ()
 *  -> acquire_contention_flag_for_unlock       -> irq_disable ()
 *  -> wake_up_task(CPU1)                       -> acquire_contention_flag_for_lock
 *      waits for CPU1 to accept interrupt          waits for CPU0 to release the contention flag
 */

/**
 * Wake up the first waiter. Returns a pointer to the task that we woke up.
 * NOTE: this function keeps the contention on the lock. mutex_lock_slowpath
 * must release it.
 */
static struct task * 
wake_up_waiter_is_self (struct mutex *mutex, struct task *self)
{
	struct mutex_waiter *w = list_item (struct mutex_waiter, list, mutex->waiters.next);
	struct task *task = w->task;

	/**
	 * Order matters. See comment in wake_up_waiter_and_disown.
	 */
	w->task = NULL;
	list_delete (&w->list);

	if (task != self)
		wake_up_task (task);

	return task;
}

/**
 * Wake up the first waiter and remove our contention on the lock.
 */
static void
wake_up_waiter_and_disown (struct mutex *mutex)
{
	if (list_empty (&mutex->waiters))
		panic ("inconsistent mutex state: wake_up_waiter_and_disown was called on a mutex with no waiters");

	struct mutex_waiter *w = list_item (struct mutex_waiter, list, mutex->waiters.next);
	struct task *task = w->task;

	/**
	 * Order matters. When the other task wakes up, it will wait for a
	 * successful acquisition of CONTENDED or w->task to become NULL. If,
	 * from its point of view, the former happens before the latter, it
	 * looks at w->task once more, and if it isn't NULL, assumes failure
	 * to acquire the lock. Therefore, set w->task to NULL before unlocking.
	 *
	 * To avoid spurious wakeups, the other task must wait for wake_up_task,
	 * which is signalled by the final store to mutex->owner_and_lock.
	 */

	w->task = NULL;
	list_delete (&w->list);
	wake_up_task (task);

	unsigned long desired = (unsigned long) task;
	if (!list_empty (&mutex->waiters))
		desired |= HAS_WAITER;
	atomic_store_release (&mutex->owner_and_lock, desired);
}

/**
 * Unlock @mutex.
 */
void
mutex_unlock (struct mutex *mutex)
{
	struct task *self = get_current_task ();

	unsigned long expected;
	if (mutex_unlock_fastpath (mutex, self, &expected))
		return;

	check_expected_owner (mutex, self, expected);

	preempt_off ();
	if (acquire_contention_flag_for_unlock (mutex, expected))
		wake_up_waiter_and_disown (mutex);
	preempt_on ();
}

/**
 * Wake up the mutex waiter because of timeout.
 */
static void
mutex_waiter_timeout (struct ktimer *timer)
{
	struct mutex_timeout_waiter *w =
		struct_parent (struct mutex_timeout_waiter, timer, timer);

	struct task *task = atomic_load_relaxed (&w->waiter.task);
	if (task)
		sched_wake__this_cpu (w->waiter.task);
}

/**
 * Lock @mutex. Slowpath.
 */
static errno_t
mutex_lock_slowpath (struct mutex *mutex, struct task *self,
	unsigned int task_state, usecs_t timeout_us, unsigned long expected)
{
	unsigned long desired;

	if (timeout_us != UINT64_MAX)
		timeout_us += us_since_boot ();

	struct mutex_timeout_waiter w;
	w.timer.callback = mutex_waiter_timeout;
	w.timer.expiry = timeout_us;
	w.timer.triggered = true;
	w.waiter.task = self;

	acquire_contention_flag_for_lock (mutex, &expected);
	if (!(expected & ~CONTENTION_FLAGS)) {
		desired = (unsigned long) self;
		atomic_store_release (&mutex->owner_and_lock, desired);
		return ESUCCESS;
	}

	/**
	 * We need to disable interrupts when self->state is dirty (anything
	 * other than TASK_RUNNING while we are still running). But we must
	 * also avoid the wake_up_task deadlock (see comment above).
	 *
	 * We must also remember to check has_pending_signal() anytime we
	 * disable IRQs.
	 */

	irq_disable ();
	if (has_pending_signal (task_state)) {
		/**
		 * There is a pending signal on us. Do not acquire the lock.
		 */
		irq_enable ();

		desired = expected & ~CONTENDED;
		if (!list_empty (&mutex->waiters))
			desired |= HAS_WAITER;

		if (!atomic_cmpxchg_acqrel_acq (&mutex->owner_and_lock, &expected, desired))
			/**
			 * If the owner_and_lock release fails, it is because
			 * mutex_unlock raised HAS_WAITER. We must now wake up
			 * one waiter, which cannot be ourselves.
			 */
			wake_up_waiter_and_disown (mutex);

		return EINTR;
	}

	list_insert_back (&mutex->waiters, &w.waiter.list);
	desired = (expected | HAS_WAITER) & ~CONTENDED;
	if (!atomic_cmpxchg_acqrel_acq (&mutex->owner_and_lock, &expected, desired)) {
		irq_enable ();

		/**
		 * If the owner_and_lock release fails, it is because
		 * mutex_unlock raised HAS_WAITER. It is now our
		 * responsibility to wake up one waiter (which might
		 * be ourselves).
		 */
		struct task *owner = wake_up_waiter_is_self (mutex, self);
		desired = (unsigned long) owner;
		if (owner == self) {
			atomic_store_release (&mutex->owner_and_lock, desired);
			return ESUCCESS;
		}

		irq_disable ();
		if (has_pending_signal (task_state)) {
			/**
			 * There is a pending signal on us. Do not acquire the
			 * lock.
			 */
			irq_enable ();
			list_delete (&w.waiter.list);
			if (!list_empty (&mutex->waiters))
				desired |= HAS_WAITER;
			atomic_store_release (&mutex->owner_and_lock, desired);
			return EINTR;
		}

		desired |= HAS_WAITER;
		atomic_store_release (&mutex->owner_and_lock, desired);
	}

	if (timeout_us != UINT64_MAX)
		ktimer_add (&w.timer);

	self->state = task_state;
	schedule ();
	irq_enable ();

	/**
	 * Whenever we are woken up, we know that any combination of:
	 * a) we successfully acquired the lock
	 * b) we timed out
	 * c) we were interrupted by a signal
	 * has happened. Even if timed_out is true, we might own the lock.
	 */
	bool timed_out = (timeout_us != UINT64_MAX) ? ktimer_remove (&w.timer) : false;

	/**
	 * Try to leave the wait list.
	 */
	for (;;) {
		if (!atomic_load_relaxed (&w.waiter.task)) {
			/**
			 * Wait for handover to complete: synchronize against
			 * wake_up_task.
			 */
			expected = (unsigned long) self;
			while ((atomic_load_acquire (&mutex->owner_and_lock) & ~CONTENTION_FLAGS) != expected)
				arch_relax ();
			return ESUCCESS;
		}

		/**
		 * Don't unnecessarily spin on cmpxchg.
		 */
		expected = atomic_load_relaxed (&mutex->owner_and_lock);
		if (expected & CONTENDED) {
			arch_relax ();
			continue;
		}

		desired = expected | CONTENDED;
		if (atomic_cmpxchg_acqrel_acq (&mutex->owner_and_lock, &expected, desired))
			break;
	}

	/**
	 * We own the lock. Now exit the wait list and unlock it.
	 */
	if (!w.waiter.task) {
		/** 
		 * We were handed the mutex before we could acquire the
		 * CONTENDED bit.
		 */
		desired = (unsigned long) self;
		if (!list_empty (&mutex->waiters))
			desired |= HAS_WAITER;
		atomic_store_relaxed (&mutex->owner_and_lock, desired);
		return ESUCCESS;
	}

	bool was_first = w.waiter.list.prev == &mutex->waiters;
	list_delete (&w.waiter.list);

	/**
	 * Unlock owner_and_lock. If this succeeds, mutex acquisition failed.
	 * Otherwise, handover happened and we must wake someone up.
	 */
	expected = desired;
	desired = expected & ~CONTENDED;
	if (atomic_cmpxchg_acqrel_acq (&mutex->owner_and_lock, &expected, desired))
		return timed_out ? ETIME : EINTR;

	/**
	 * If we were the first mutex waiter, we might as well wake up ourselves.
	 */
	if (was_first) {
		desired = (unsigned long) self;
		if (!list_empty (&mutex->waiters))
			desired |= HAS_WAITER;
		atomic_store_release (&mutex->owner_and_lock, desired);
		return ESUCCESS;
	}

	/**
	 * Wake someone else up an return an error.
	 */
	wake_up_waiter_and_disown (mutex);
	return timed_out ? ETIME : EINTR;
}

/**
 * Lock @mutex. Common part.
 */
static errno_t
mutex_lock_common (struct mutex *mutex, usecs_t timeout_us, bool interruptible)
{
	struct task *self = get_current_task ();
	unsigned int task_state = interruptible ? TASK_INTERRUPTIBLE : TASK_UNINTERRUPTIBLE;

	unsigned long expected;
	if (mutex_lock_fastpath (mutex, self, &expected))
		return ESUCCESS;

	if (!timeout_us)
		return EAGAIN;

	if (!irqs_enabled ())
		panic ("mutex_lock was called with interrupts disabled!");

	preempt_off ();
	errno_t result = mutex_lock_slowpath (mutex, self, task_state, timeout_us, expected);
	preempt_on ();
	return result;
}

void
mutex_lock (struct mutex *mutex)
{
	if (mutex_lock_common (mutex, UINT64_MAX, false) != ESUCCESS)
		panic ("mutex_lock (infallible variant): acquisition failed");
}

errno_t
mutex_lock_interruptible (struct mutex *mutex)
{
	return mutex_lock_common (mutex, UINT64_MAX, true);
}

errno_t
mutex_lock_timeout (struct mutex *mutex, usecs_t us)
{
	return mutex_lock_common (mutex, us, false);
}

errno_t
mutex_lock_timeout_interruptible (struct mutex *mutex, usecs_t us)
{
	return mutex_lock_common (mutex, us, true);
}
