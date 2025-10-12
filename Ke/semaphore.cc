// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/semaphore.cc
 * Semaphore objects.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ke/sched.h>
#include <Ke/semaphore.h>
#include <Ke/thread.h>
#include <Ke/turnstile.h>

void KeSemaphore::signal_slowpath(void)
{
	KTURNSTILE *ts = KeTurnstileGet(&m_value);
	atomic_fetch_and(&m_value, ~SIGN_BIT, _MO_Relaxed);
	KeTurnstileWake(ts, KTURNSTILE_Q_WRITER);
	KeTurnstilePut(ts);
}

static bool has_pending_signal(void)
{
	// FIXME: signal handling...
	return false;
}

OSSTATUS KeSemaphore::wait_slowpath(
	unsigned int old,
	bool interruptible,
	nsec_t timeout)
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

retry:
	while (old & ~SIGN_BIT) {
		unsigned int des = old - 1;
		if (atomic_cmpxchg_weak_for_lock(&m_value, &old, des)) {
			[[likely]];
			return OS_STATUS_SUCCESS;
		}
	}

	/*
	 * Check if we timed out or are interrupted by a pending signal.
	 */
	if (!timeout || (timeout != NSEC_MAX && HalReadSchedClock() >= timeout))
		return OS_STATUS_SEMA_WAIT_TIMED_OUT;
	if (interruptible && has_pending_signal())
		return OS_STATUS_SEMA_WAIT_INTERRUPTED;

	/*
	 * Acquire a turnstile, then try to set the SIGN_BIT indicating that
	 * there are waiters.
	 */
	KTURNSTILE *ts = KeTurnstileGet(&m_value);
	old = atomic_load_relaxed(&m_value);
	for (;;) {
		if (old & ~SIGN_BIT) {
			/*
			 * The semaphore was concurrently signaled. Try to grab
			 * it fast.
			 */
			[[unlikely]];
			KeTurnstilePut(ts);
			goto retry;
		}

		if (old & SIGN_BIT)
			break;

		bool status = atomic_cmpxchg_weak(
			&m_value,
			&old,
			old | SIGN_BIT,
			_MO_Relaxed,
			_MO_Relaxed
		);

		if (status)
			break;
	}

	KeSetCurrentState(sleep_state);
	KeTurnstileBlock(ts, KTURNSTILE_Q_WRITER);
	KeTurnstilePut(ts);
	old = atomic_load_relaxed(&m_value);
	goto retry;
}

