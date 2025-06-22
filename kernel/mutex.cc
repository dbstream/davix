/**
 * Kernel mutexes.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/irql.h>
#include <davix/atomic.h>
#include <davix/mutex.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/task.h>
#include <uapi/davix/errno.h>

static constexpr nsecs_t NO_TIMEOUT = -1ULL;

static constexpr uintptr_t MUTEX_WAITER		= uintptr_t(1) << 0;
static constexpr uintptr_t MUTEX_LOCK		= uintptr_t(1) << 1;
static constexpr uintptr_t MUTEX_PTR		= ~(MUTEX_WAITER | MUTEX_LOCK);

struct mutex_op_state {
	uintptr_t lockval;
	Task *self;
};

/*
 * Mutex fastpaths.  These are forcibly inlined.
 */

static inline bool
lock_fastpath [[gnu::always_inline]] (Mutex *mtx, mutex_op_state &state)
{
	state.lockval = 0;

	uintptr_t desired = (uintptr_t) state.self;
	bool ret = atomic_cmpxchg_weak (&mtx->m_owner_and_flags,
			&state.lockval, desired,
			mo_acquire, mo_relaxed);

	return ret;
}

static inline bool
unlock_fastpath [[gnu::always_inline]] (Mutex *mtx, mutex_op_state &state)
{
	state.lockval = (uintptr_t) state.self;

	uintptr_t desired = 0;
	bool ret = atomic_cmpxchg_weak (&mtx->m_owner_and_flags,
			&state.lockval, desired,
			mo_release, mo_relaxed);

	return ret;
}

/*
 * Mutex slowpaths.  Never inlined.
 */

static int
lock_slowpath [[gnu::noinline]] (Mutex *mtx, mutex_op_state &state,
		bool interruptible, nsecs_t timeout);

static void
unlock_slowpath [[gnu::noinline]] (Mutex *mtx, mutex_op_state &state);

/**
 * Mutex::trylock - try to lock the mutex without waiting.
 * Returns true if the mutex was successfully locked by us.
 */
bool
Mutex::trylock (void)
{
	mutex_op_state state = { 0, get_current_task () };
	if (lock_fastpath (this, state)) [[likely]] {
		return true;
	}

	return lock_slowpath (this, state, false, 0) == 0;
}

/**
 * Mutex::lock - lock the mutex.
 * Always returns with the mutex locked and held by us.
 */
void
Mutex::lock (void)
{
	mutex_op_state state = { 0, get_current_task () };
	if (lock_fastpath (this, state)) [[likely]] {
		return;
	}

	int ret = lock_slowpath (this, state, false, NO_TIMEOUT);
	if (ret)
		panic ("Mutex::lock: slowpath failed to acquire the lock!");
}

/**
 * Mutex::lock_interruptible - lock the mutex.
 * Returns zero for success or EINTR if we were interrupted by a signal.
 */
int
Mutex::lock_interruptible (void)
{
	mutex_op_state state = { 0, get_current_task () };
	if (lock_fastpath (this, state)) [[likely]] {
		return 0;
	}

	return lock_slowpath (this, state, true, NO_TIMEOUT);
}

/**
 * Mutex::lock_timeout - lock the mutex.
 * @ns: timeout in nanoseconds
 * Returns zero for success or ETIME if we timed out.
 */
int
Mutex::lock_timeout (nsecs_t ns)
{
	mutex_op_state state = { 0, get_current_task () };
	if (lock_fastpath (this, state)) [[likely]] {
		return 0;
	}

	return lock_slowpath (this, state, false, ns);
}

/**
 * Mutex::lock_timeout_interruptible - lock the mutex.
 * @ns: timeout in nanoseconds
 * Returns zero for success, ETIME if we timed out, or EINTR if we were
 * interrupted by a signal.
 */
int
Mutex::lock_timeout_interruptible (nsecs_t ns)
{
	mutex_op_state state = { 0, get_current_task () };
	if (lock_fastpath (this, state)) [[likely]] {
		return 0;
	}

	return lock_slowpath (this, state, true, ns);
}

/**
 * Mutex::unlock - unlock the mutex.
 */
void
Mutex::unlock (void)
{
	mutex_op_state state = { 0, get_current_task () };
	if (unlock_fastpath (this, state)) [[likely]] {
		return;
	}

	unlock_slowpath (this, state);
}

static void
spin_on_lock_bit (Mutex *mtx, mutex_op_state &state)
{
	do {
		smp_spinlock_hint ();
		state.lockval = atomic_load_relaxed (&mtx->m_owner_and_flags);
	} while (state.lockval & MUTEX_LOCK);
}

static int
lock_slowpath [[gnu::noinline]] (Mutex *mtx, mutex_op_state &state,
		bool interruptible, nsecs_t ns)
{
	nsecs_t expiry = ns;
	if (ns && ns != NO_TIMEOUT)
		expiry += ns_since_boot ();

	int wait_state = interruptible
		? TASK_INTERRUPTIBLE
		: TASK_UNINTERRUPTIBLE;

retry:
	if (state.lockval & MUTEX_LOCK) {
		if (!ns)
			/*
			 * For trylock, don't bother spinning on the lock bit.
			 */
			return ETIME;
		spin_on_lock_bit (mtx, state);
	}

	if (!(state.lockval & MUTEX_PTR)) {
		/*
		 * Attempt acquiring the mutex.
		 */
		uintptr_t desired = (uintptr_t) state.self
				| (state.lockval & MUTEX_WAITER);

		bool ret = atomic_cmpxchg_weak (&mtx->m_owner_and_flags,
				&state.lockval, desired,
				mo_acquire, mo_relaxed);

		if (ret)
			/*
			 * Installing ourselves into m_owner_and_flags means we
			 * successfully acquired the mutex.
			 */
			return 0;

		/*
		 * Go back to the beginning and retry locking the mutex.
		 *
		 * There are multiple reasons we do this:
		 * 1. We use atomic_cmpxchg_weak above, which means that the
		 *    acquire may spuriously fail.
		 * 2. If someone has set the lock bit, when they release it, the
		 *    mutex might still be unclaimed.
		 */
		goto retry;
	}

	if (!ns)
		/*
		 * This is trylock, don't bother looking at ns_since_boot.
		 */
		return ETIME;

	if (expiry != NO_TIMEOUT && ns_since_boot () >= expiry)
		/*
		 * We timed out.
		 */
		return ETIME;

	if (interruptible && has_pending_signal ())
		/*
		 * There is a pending signal and this is an interruptible wait.
		 */
		return EINTR;

	/*
	 * Now we want to insert ourselves into the waiter chain of this lock.
	 * To do this, we must set the lock bit.
	 */
	MutexWaiter waiter;
	waiter.task = state.self;
	waiter.ticket = sched_get_blocking_ticket ();

	uintptr_t desired = state.lockval | MUTEX_LOCK;

	disable_dpc ();
	bool ret = atomic_cmpxchg_weak (&mtx->m_owner_and_flags,
			&state.lockval, desired,
			mo_acquire, mo_relaxed);
	if (!ret) {
		/*
		 * We failed to set the lock bit; go back to the beginning and
		 * retry locking the mutex.
		 */
		enable_dpc ();
		goto retry;
	}

	mtx->m_waiters.push_back (&waiter);

retry_sleep:
	desired &= ~MUTEX_LOCK;
	desired |= MUTEX_WAITER;

	/*
	 * Release the lock bit.
	 */
	atomic_store_release (&mtx->m_owner_and_flags, desired);

	/*
	 * Fall asleep.
	 */
	sched_timeout_ticket (expiry, wait_state, waiter.ticket);

	/*
	 * We woke up.  This is because of:
	 * 1. Someone released the lock and woke us up.
	 * 2. We timed out.
	 * 3. Our sleep was disturbed by a pending signal.
	 * In either of these cases, we must try to acquire the lock
	 * before we do anything else.
	 */
	state.lockval = MUTEX_WAITER;
retry_nosleep:
	desired = (uintptr_t) state.self | MUTEX_LOCK | MUTEX_WAITER;
	do {
		ret = atomic_cmpxchg_weak (&mtx->m_owner_and_flags,
				&state.lockval, desired,
				mo_acquire, mo_relaxed);
		if (ret) {
			/*
			 * We successfully got the mutex.  Now remove ourselves
			 * from the waiter list and return successfully.
			 */
			waiter.entry.remove ();
			desired &= ~MUTEX_LOCK;
			if (mtx->m_waiters.empty ())
				desired &= ~MUTEX_WAITER;
			atomic_store_release (&mtx->m_owner_and_flags, desired);
			enable_dpc ();
			return 0;
		}

		enable_dpc ();
		if (state.lockval & MUTEX_LOCK) {
			/*
			 * Wait for the lock bit to be cleared before we try
			 * again.
			 */
			spin_on_lock_bit (mtx, state);
		}

		disable_dpc ();
	} while (!(state.lockval & MUTEX_PTR));

	/*
	 * We woke up, but someone else raced with us to acquire the lock.  We
	 * must still set the lock bit and update waiter.ticket or remove
	 * ourselves from the waiter list if we timed out or were interrupted.
	 */
	do {
		desired = state.lockval | MUTEX_LOCK;
		ret = atomic_cmpxchg_weak (&mtx->m_owner_and_flags,
				&state.lockval, desired,
				mo_acquire, mo_relaxed);
		if (ret) {
			/*
			 * We successfully set the lock bit of the mutex.
			 *
			 * Either go back to sleep, or remove ourselves from the
			 * waiter list and return if we timed out or were
			 * interrupted.
			 */
			int etime_or_eintr = 0;
			if (expiry != NO_TIMEOUT && ns_since_boot () >= expiry)
				etime_or_eintr = ETIME;
			else if (interruptible && has_pending_signal ())
				etime_or_eintr = EINTR;

			if (!etime_or_eintr) {
				/*
				 * We did not time out, and we did not have a
				 * pending signal.
				 */
				waiter.ticket = sched_get_blocking_ticket ();
				goto retry_sleep;
			}

			/*
			 * We timed out or were interrupted.  Remove ourselves
			 * from the waiter list and return with an error.
			 */
			waiter.entry.remove ();
			desired &= ~MUTEX_LOCK;
			if (mtx->m_waiters.empty ())
				desired &= ~MUTEX_WAITER;
			atomic_store_release (&mtx->m_owner_and_flags, desired);
			enable_dpc ();
			return etime_or_eintr;
		}

		enable_dpc ();
		if (state.lockval & MUTEX_LOCK) {
			/*
			 * Wait for the lock bit to be cleared before we try
			 * again.
			 */
			spin_on_lock_bit (mtx, state);
		}

		disable_dpc ();
	} while (state.lockval & MUTEX_PTR);
	goto retry_nosleep;
}

static void
unlock_slowpath [[gnu::noinline]] (Mutex *mtx, mutex_op_state &state)
{
retry:
	if (state.lockval & MUTEX_LOCK) {
		/*
		 * Wait for the lock bit to be cleared.
		 */
		spin_on_lock_bit (mtx, state);
	}

	if (!(state.lockval & MUTEX_WAITER)) {
		/*
		 * Try fast-releasing the mutex.
		 */
		bool ret = atomic_cmpxchg_weak (&mtx->m_owner_and_flags,
				&state.lockval, 0,
				mo_release, mo_relaxed);

		if (ret)
			/*
			 * We successfully released the mutex and we don't have
			 * to wake up any task.
			 */
			return;

		/*
		 * Go back to the beginning and retry releasing the mutex.  We
		 * use atomic_cmpxchg_weak above, which means that the release
		 * may spuriously fail.
		 */
		goto retry;
	}

	/*
	 * Try to acquire the lock bit now.
	 */
	disable_dpc ();
	bool ret = atomic_cmpxchg_weak (&mtx->m_owner_and_flags,
			&state.lockval, MUTEX_WAITER | MUTEX_LOCK,
			mo_acquire, mo_relaxed);
	if (!ret) {
		/*
		 * We failed to set the lock bit; go back to the beginning and
		 * retry it all.
		 */
		enable_dpc ();
		goto retry;
	}

	MutexWaiter *waiter = *mtx->m_waiters.begin ();
	Task *task = waiter->task;
	sched_ticket_t ticket = waiter->ticket;

	atomic_store_release (&mtx->m_owner_and_flags, MUTEX_WAITER);
	sched_wake (task, ticket);
	enable_dpc ();
}

