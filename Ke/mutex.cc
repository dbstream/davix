// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/mutex.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ke/mutex.h>
#include <Ke/thread.h>
#include <Ke/turnstile.h>
#include <davix/bug.h>

void KeMutex::unlock_slowpath(unsigned char lockval)
{
	while (lockval == 1) {
		bool status = atomic_cmpxchg_weak(
			&m_lockval,
			&lockval,
			0,
			_MO_Release,
			_MO_Relaxed
		);

		if (status)
			return;
	}

	/*
	 * This BUG_ON can be reached if someone attempts to release a mutex
	 * which is not currently held by anyone, or if the mutex is otherwise
	 * in a corrupt state.
	 */
	BUG_ON(lockval != 3);

	KTURNSTILE *ts = KeTurnstileGet(this);
	atomic_store_release(&m_lockval, 0);
	// NB: wakes ALL waiters.
	KeTurnstileWake(ts, KTURNSTILE_Q_WRITER);
	KeTurnstilePut(ts);
}

static bool has_pending_signal(void)
{
	// FIXME: signal handling...
	return false;
}

OSSTATUS KeMutex::lock_slowpath(
	unsigned char lockval,
	bool interruptible,
	nsec_t timeout
)
{
	int sleep_state = (interruptible)
		? KTHREAD_INTERRUPTIBLE
		: KTHREAD_UNINTERRUPTIBLE;

	/*
	 * Setup the thread for timeout sleep if there is a timeout.
	 */
	if (timeout && timeout != NSEC_MAX) {
		timeout += HalReadSchedClock();
		KeSetSleepTimeoutNanosAbs(timeout);
		sleep_state |= KTHREAD_TIMEOUT_F;
	}

	/*
	 * Try to acquire the mutex instantly if it is free.
	 */
	while (lockval == 0) {
retry:
		bool status = atomic_cmpxchg_weak_for_lock(
			&m_lockval,
			&lockval,
			1
		);

		if (status)
			return OS_STATUS_SUCCESS;
	}

	/*
	 * Check if we timed out or are interrupted by a pending signal.
	 */
	if (!timeout || (timeout != NSEC_MAX && HalReadSchedClock() >= timeout))
		return OS_STATUS_LOCK_ACQUIRE_TIMED_OUT;
	if (interruptible && has_pending_signal())
		return OS_STATUS_LOCK_ACQUIRE_INTERRUPTED;

	/*
	 * Acquire a turnstile, then reload lockval.  We will set the pending
	 * bit if it is not already set, but we must be careful to handle the
	 * case where someone simultaneously unlocks the mutex.
	 */
	KTURNSTILE *ts = KeTurnstileGet(this);
	lockval = atomic_load_relaxed(&m_lockval);
	for (;;) {
		/*
		 * If the mutex was concurrently unlocked, release the turnstile
		 * and try again.
		 */
		if (lockval == 0) {
			KeTurnstilePut(ts);
			goto retry;
		}

		/*
		 * We do not need to set the pending bit if it is already set.
		 * Mutex unlock will spin on KeTurnstileGet until we enter
		 * KeTurnstileBlock and it internally unlocks the turnstile.
		 */
		if (lockval == 3)
			break;

		/*
		 * The permitted mutex states are:
		 * 0 means unlocked
		 * 1 means locked, no waiters
		 * 3 means locked, with waiters
		 */
		BUG_ON(lockval != 1);

		bool status = atomic_cmpxchg_weak(
			&m_lockval,
			&lockval,
			3,
			_MO_Relaxed,
			_MO_Relaxed
		);

		if (status)
			break;
	}

	KeSetCurrentState(sleep_state);
	KeTurnstileBlock(ts, KTURNSTILE_Q_WRITER);
	KeTurnstilePut(ts);
	lockval = 0;
	goto retry;
}

