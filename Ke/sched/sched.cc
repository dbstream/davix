// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/sched/sched.cc
 * Source code for the Davix scheduler.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ex/thread.h>
#include <Hal/percpu.h>
#include <Ke/context.h>
#include <Ke/sched.h>
#include <Ke/smp.h>
#include <Ke/spinlock.h>
#include <Ke/time.h>
#include <Ke/timer.h>
#include <Ke/thread.h>
#include <Ki/sched.h>
#include <davix/bug.h>
#include <dsl/hlist.h>
#include <container_of.h>

/*
 * vruntime_scale: scale factors for vruntime.
 *
 * Tasks with higher priority have a smaller scale factor, which means that they
 * consume their timeslice at a slower pace.
 *
 * FIXME: tune these numbers.
 */
static const unsigned int vruntime_scale[KPRIORITY_MAX_CFS + 1] = {
	24,
	21,
	18,
	16,
	14,
	12,
	10,
	9,
	8,
	7,
};

/*
 * VRUNTIME_PREEMPT: vruntime slice length.  When the delta between the
 * currently running thread's vruntime and the minimum vruntime in cfs_tasks
 * becomes greater than or equal to this, preempt.
 */
static constexpr unsigned long long VRUNTIME_PREEMPT = 24ULL * 5000000ULL;

/**
 * KiSetupInitialThreadContext - setup kiProcessorContext of a new thread.
 */
void KiSetupInitialThreadContext(void)
{
	/*
	 * Enable IRQs, but leave DPCs and preemption disabled with count = 1.
	 * DPCs will be enabled by KiFinalizeTaskSwitch, and preemption is later
	 * enabled by the HAL thread entry routine.
	 */

	unsigned int old, new_;

	new_ = 1U | KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	old = HalExchangePerCPU(kiProcessorContext.irq_counter, new_);
	if (!(old & KI_CONTEXT_COUNTER_EVENT_NOT_PENDING))
		KeDispatchPendingIRQs();
	KeEnableIRQs();

	new_ = 1U | KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	old = HalExchangePerCPU(kiProcessorContext.dpc_counter, new_);
	if (!(old & KI_CONTEXT_COUNTER_EVENT_NOT_PENDING))
		KeSetPendingDPC();

	new_ = 1U | KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	old = HalExchangePerCPU(kiProcessorContext.preemption_counter, new_);
	if (!(old & KI_CONTEXT_COUNTER_EVENT_NOT_PENDING))
		KeSetPendingPreemption();
}

struct VRuntimeComparator {
	constexpr bool operator()(const KTHREAD *lhs, const KTHREAD *rhs) const
	{
		return lhs->vruntime < rhs->vruntime;
	}
};

typedef dsl::TypedList<KTHREAD, &KTHREAD::rq_list> RQList;
typedef dsl::TypedAVLTree<KTHREAD, &KTHREAD::rq_avl, VRuntimeComparator> RQTree;

struct KSCHEDULER {
	KeDPCSpinlock scheduler_lock;
	KTHREAD *next_thread;
	KTHREAD *current_thread;
	KTHREAD *idle_task;

	RQList realtime_tasks;
	RQTree cfs_tasks;

	unsigned long num_realtime;
	unsigned long num_cfs;

	KTIMER preempt_timer;
	nsec_t current_deadline;

	nsec_t new_deadline;
	bool deadline_dirty;
};

static DEFINE_PERCPU(KSCHEDULER, kiScheduler);

static void handle_preempt_timer(KTIMER *timer);

HAL_PERCPU_CALLBACK(cpu)
{
	KSCHEDULER *sched = HalPtrPerCPU(kiScheduler, cpu);

	sched->scheduler_lock.init();
	sched->next_thread = nullptr;
	sched->current_thread = nullptr;
	sched->idle_task = nullptr;
	sched->realtime_tasks.init();
	sched->cfs_tasks.init();
	sched->num_realtime = 0;
	sched->num_cfs = 0;
	sched->preempt_timer.init(handle_preempt_timer);
	sched->current_deadline = NSEC_MAX;
	sched->new_deadline = 0;
	sched->deadline_dirty = false;
}

static KTHREAD *context_switch(KTHREAD *previous, KTHREAD *next)
{
	unsigned int old;
	unsigned int pre = HalReadPerCPU(kiProcessorContext.preemption_counter);
	unsigned int dpc = HalReadPerCPU(kiProcessorContext.dpc_counter);
	unsigned int irq = HalReadPerCPU(kiProcessorContext.irq_counter);

	previous = HalSwitchThread(previous, next);

	irq |= KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	dpc |= KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	pre |= KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;

	old = HalExchangePerCPU(kiProcessorContext.irq_counter, irq);
	if (!(old & KI_CONTEXT_COUNTER_EVENT_NOT_PENDING)) {
		if (!(irq & ~KI_CONTEXT_COUNTER_EVENT_NOT_PENDING))
			KeDispatchPendingIRQs();
		else
			KeSetPendingIRQ();
	}

	/*
	 * NOTE: context_switch happens under KSCHEDULER::scheduler_lock and
	 * under KeDisablePreemption, which means that we will never dispatch
	 * DPCs or preemption immediately when restoring these.
	 */

	old = HalExchangePerCPU(kiProcessorContext.dpc_counter, dpc);
	if (!(old & KI_CONTEXT_COUNTER_EVENT_NOT_PENDING))
		KeSetPendingDPC();

	old = HalExchangePerCPU(kiProcessorContext.preemption_counter, pre);
	if (!(old & KI_CONTEXT_COUNTER_EVENT_NOT_PENDING))
		KeSetPendingPreemption();

	return previous;
}

static void set_sched_timer(KSCHEDULER *scheduler, nsec_t deadline)
{
	BUG_ON(deadline == NSEC_MAX);

	if (scheduler->current_deadline != NSEC_MAX) {
		BUG_ON(!KeUnsetTimer(&scheduler->preempt_timer));
	}

	BUG_ON(!KeSetTimer(&scheduler->preempt_timer, deadline));
	scheduler->current_deadline = deadline;
}

static void unset_sched_timer(KSCHEDULER *scheduler)
{
	if (scheduler->current_deadline != NSEC_MAX) {
		BUG_ON(!KeUnsetTimer(&scheduler->preempt_timer));
		scheduler->current_deadline = NSEC_MAX;
	}
}

static nsec_t calc_vruntime_deadline(KTHREAD *current, KTHREAD *next, int prio)
{
	unsigned long long vruntime = next->vruntime
			+ VRUNTIME_PREEMPT
			- current->vruntime;

	nsec_t vruntime_ns = vruntime / vruntime_scale[prio];

	return current->last_vruntime_update + vruntime_ns;
}

/**
 * do_enqueue_locked - put @thread on the runqueue of @scheduler.
 * @scheduler: target CPU's KSCHEDULER
 * @thread: the thread to enqueue
 * Return value:
 * - 0: no further work
 * - 1: preempt_timer is wrong and must be updated
 * - 2: @thread preempts the currently-running thread on @scheduler.
 *
 * The scheduler_lock on @scheduler must be held.
 */
static int do_enqueue_locked(KSCHEDULER *scheduler, KTHREAD *thread)
{
	bool want_preemption = false;
	KTHREAD *current = scheduler->current_thread;
	int current_prio = current->current_priority;
	int prio = thread->current_priority;

	if (current == scheduler->idle_task) {
		/*
		 * The idle thread should always be preempted.
		 */
		want_preemption = true;
	} else if (prio > KPRIORITY_MAX_CFS && current_prio < prio) {
		/*
		 * A realtime thread always preempts a non-realtime thread.
		 */
		want_preemption = true;
	} else if (current_prio <= KPRIORITY_MAX_CFS) {
		/*
		 * Both threads are CFS threads.  If the vruntime delta is
		 * greater than or equal to VRUNTIME_PREEMPT, we want immediate
		 * preemption.
		 */
		if (thread->vruntime + VRUNTIME_PREEMPT <= current->vruntime)
			want_preemption = true;
	}

	/*
	 * If we want a preemption, try to set next_thread.  But if next_thread
	 * is already set to some other thread, we must decide which thread gets
	 * to run first and enqueue the other thread normally.
	 */
	if (want_preemption) {
		if (!scheduler->next_thread) {
			scheduler->next_thread = thread;
			return 2;
		}

		KTHREAD *next = scheduler->next_thread;
		if (prio > KPRIORITY_MAX_CFS && next->current_priority < prio) {
			/*
			 * Our thread is a realtime thread and the other thread
			 * is a CFS thread.
			 */
			scheduler->next_thread = thread;
			thread = next;
			prio = thread->current_priority;
		} else if (next->current_priority <= KPRIORITY_MAX_CFS) {
			/*
			 * Both threads are CFS thread.  Let the thread with the
			 * least vruntime execute first.
			 */
			if (thread->vruntime < next->vruntime) {
				scheduler->next_thread = thread;
				thread = next;
				prio = thread->current_priority;
			}
		}
	}

	if (prio > KPRIORITY_MAX_CFS) {
		scheduler->realtime_tasks.push_back(thread);
		/*
		 * No further work needed, as we only get here if the CPU is
		 * already running a realtime thread or next_thread is also a
		 * realtime thread.
		 */
		return 0;
	}

	scheduler->cfs_tasks.insert(thread);
	if (want_preemption)
		/*
		 * No further work needed, since we only get here if next_thread
		 * was set, and, in that case, someone else is setting or has
		 * set the pending preemption flag already.
		 */
		return 0;

	if (scheduler->next_thread)
		/*
		 * If next_thread is set, someone else is setting or has set the
		 * pending preemption flag already, which means that the
		 * preempt_timer will be reset anyways.
		 */
		return 0;

	thread = scheduler->cfs_tasks.first();
	BUG_ON(!thread);

	/*
	 * Check if the deadline has changed.
	 */
	nsec_t deadline = calc_vruntime_deadline(current, thread, current_prio);
	if (deadline != scheduler->current_deadline) {
		/*
		 * Unconditionally update new_deadline.
		 */
		scheduler->new_deadline = deadline;

		/*
		 * If deadline_dirty is already set, it means that we need not
		 * do any further work for the preempt_timer to be refreshed.
		 */
		if (!scheduler->deadline_dirty) {
			scheduler->deadline_dirty = true;
			return 1;
		}
	}

	return 0;
}

static void do_wakeup_local(KTHREAD *thread)
{
	KSCHEDULER *scheduler = HalPtrThisCpu(kiScheduler);
	scheduler->scheduler_lock.lock();

	int status = do_enqueue_locked(scheduler, thread);

	if (status == 1) {
		set_sched_timer(scheduler, scheduler->new_deadline);
		scheduler->deadline_dirty = false;
	} else if (status == 2) {
		unset_sched_timer(scheduler);
		scheduler->deadline_dirty = false;
		KeSetPendingPreemption();
	}

	scheduler->scheduler_lock.unlock();
}

/**
 * KeWakeThread - wake up a sleeping KTHREAD.
 * @thread: KTHREAD to wake up
 * Returns true if we woke up the KTHREAD, false if it was already running.
 */
bool KeWakeThread(KTHREAD *thread)
{
	int state = atomic_load_relaxed(&thread->thread_state);
	/*
	 * Try to set KTHREAD_WAKING_UP in a loop for as long as the thread has
	 * not been claimed for wakeup by someone else or is already running.
	 */
	for (;;) {
		/*
		 * If state is KTHREAD_RUNNING, the thread is already running,
		 * and if it is KTHREAD_WAKING_UP, someone else has claimed the
		 * thread for wakeup.  Either way, our work ends here.
		 */
		if (state == KTHREAD_RUNNING || state == KTHREAD_WAKING_UP)
			return false;

		KeDisablePreemption();
		atomic_fetch_inc(&thread->active_wakeup_count, _MO_Relaxed);
		bool status = atomic_cmpxchg(
			&thread->thread_state,
			&state,
			KTHREAD_WAKING_UP,
			_MO_SeqCst,
			_MO_Relaxed
		);

		if (status) {
			[[likely]];
			break;
		}

		atomic_fetch_dec(&thread->active_wakeup_count, _MO_Release);
		KeEnablePreemption();
	}

	/*
	 * Look at the value of KTHREAD::running _after_ we have successfully
	 * set KTHREAD_WAKING_UP.  Since KiFinalizeTaskSwitch clears running
	 * _before_ looking for KTHREAD_WAKING_UP state, this means that if we
	 * see running==1, we know that KiFinalizeTaskSwitch will see our store
	 * of KTHREAD_WAKING_UP to KTHREAD::thread_state above.
	 *
	 * Therefore it is safe to return without enqueueing the thread on a
	 * runqueue if we see running==1, because KiFinalizeTaskSwitch will do
	 * it on our behalf.  Return true because it was effectively this call
	 * to KeWakeThread that woke the thread up, although we're not the one
	 * to enqueue it.
	 */
	if (atomic_load(&thread->running, _MO_SeqCst)) {
		atomic_fetch_dec(&thread->active_wakeup_count, _MO_Relaxed);
		KeEnablePreemption();
		return true;
	}

	/*
	 * We observed running==0 which means that we don't know if
	 * KiFinalizeTaskSwitch sees our store to thread_state.  Whichever of
	 * our cmpxchg and KiFinalizeTaskSwitch's cmpxchg succeeds decides who
	 * will enqueue the thread.
	 *
	 * _MO_Relaxed ordering is sufficient, because the KTHREAD_WAKING_UP
	 * value we would observe if our cmpxchg succeeds is our own store from
	 * above.
	 */
	state = KTHREAD_WAKING_UP;
	bool status = atomic_cmpxchg(
		&thread->thread_state,
		&state,
		KTHREAD_RUNNING,
		_MO_Relaxed,
		_MO_Relaxed
	);

	/*
	 * If our cmpxchg failed, it means that KiFinalizeTaskSwitch succeeded,
	 * which means we don't have to do any more work.
	 */
	if (!status) {
		atomic_fetch_dec(&thread->active_wakeup_count, _MO_Release);
		KeEnablePreemption();
		return true;
	}

	/*
	 * Wait for active_wakeup_count to become zero before proceeding.  This
	 * ensures no one else (including KiFinalizeTaskSwitch) is concurrently
	 * using the KTHREAD's fields without actively holding a reference to
	 * the thread.
	 */
	if (atomic_dec_fetch(&thread->active_wakeup_count, _MO_Acquire) != 0) {
		[[unlikely]];
		do {
			smp_spinwait_hint();
		} while(atomic_load_acquire(&thread->active_wakeup_count) != 0);
	}

	/*
	 * Now put it on a runqueue.
	 */

	/*
	 * FIXME: we don't migrate threads when waking them up yet.  Instead, we
	 * always put them on the current processor's runqueue.
	 */
	do_wakeup_local(thread);

	KeEnablePreemption();
	return true;
}

/**
 * KiFinalizeTaskSwitch - finalize a task switch from @previous.
 */
void KiFinalizeTaskSwitch(KTHREAD *previous)
{
	KSCHEDULER *scheduler = HalPtrThisCpu(kiScheduler);
	KTHREAD *current = HalCurrentThread();
	unsigned int prio = current->current_priority;

	atomic_fetch_inc(&previous->active_wakeup_count, _MO_Relaxed);

	/*
	 * Set previous->running to false _before_ looking at thread_state.
	 * This ordering is necessary because otherwise we might miss a store
	 * of KTHREAD_WAKING_UP to thread_state by KeWakeThread.
	 */
	atomic_store(&previous->running, false, _MO_SeqCst);

	/*
	 * Now look at thread_state.  Do an atomic cmpxchg on thread_state from
	 * KTHREAD_WAKING_UP to KTHREAD_RUNNING:
	 * - If it succeeds, we "won" the race against KeWakeThread and we must
	 *   wake the thread up.
	 * - Otherwise, KeWakeThread will wake the thread up.
	 */
	int state = KTHREAD_WAKING_UP;
	bool status = atomic_cmpxchg(
		&previous->thread_state,
		&state,
		KTHREAD_RUNNING,
		_MO_SeqCst,
		_MO_SeqCst
	);

	if (!status) {
		[[likely]];
		atomic_fetch_dec(&previous->active_wakeup_count, _MO_Release);

		if (state != KTHREAD_RUNNING) {
			/*
			 * The idle task must not block.
			 */
			BUG_ON(previous == scheduler->idle_task);
		}
	} else {
		/*
		 * Wait for active_wakeup_count to become zero before we
		 * proceed.  This ensures KeWakeThread is finished using the
		 * KTHREAD.
		 */
		if (atomic_dec_fetch(
				&previous->active_wakeup_count, _MO_Acquire
		) != 0) {
			[[unlikely]];
			do {
				smp_spinwait_hint();
			} while (atomic_load_acquire(
				&previous->active_wakeup_count
			) != 0);
		}

		/*
		 * We are currently holding the lock on a KSCHEDULER (the
		 * current processor's).  Therefore, attempting thread migration
		 * here is likely to end up with difficult to debug deadlock
		 * scenarios and is just generally annoying.  The simplest
		 * solution is to just always enqueue the thread on our own
		 * runqueue.
		 */

		if (previous->current_priority > KPRIORITY_MAX_CFS) {
			/*
			 * The thread is a realtime thread.  We know, because we
			 * just rescheduled, that next_thread is NULL, so if the
			 * current thread is not a realtime thread, simply set
			 * next_thread.
			 */

			if (prio <= KPRIORITY_MAX_CFS) {
				scheduler->next_thread = previous;
				KeSetPendingPreemption();
			} else
				scheduler->realtime_tasks.push_back(previous);

			goto out_unset_timer;
		} else {
			/*
			 * The thread is a CFS thread.  Insert it into the
			 * vruntime tree, if we should switch back to it
			 * immediately it will be caught by the below code.
			 */
			scheduler->cfs_tasks.insert(previous);
			scheduler->num_cfs++;
		}
	}

	/*
	 * Calculate when the next preemption event should happen.  Note that we
	 * don't consider realtime threads here, which is fine because:
	 * - If a realtime thread was on the runqueue, we would've switched to
	 *   it, and realtime threads are never preempted.
	 * - Otherwise, no realtime threads are on the runqueue and they need
	 *   not be considered here.
	 */

	if (prio <= KPRIORITY_MAX_CFS) {
		KTHREAD *next = scheduler->cfs_tasks.first();
		if (next) {
			nsec_t deadline = calc_vruntime_deadline(
				current,
				next,
				prio
			);

			set_sched_timer(scheduler, deadline);
			scheduler->deadline_dirty = false;
			goto out_unlock;
		}
	}

out_unset_timer:
	unset_sched_timer(scheduler);
	scheduler->deadline_dirty = false;
out_unlock:
	scheduler->scheduler_lock.unlock();
}

static void insert_task(KSCHEDULER *scheduler, KTHREAD *task, int priority)
{
	if (priority == KPRIORITY_REALTIME) {
		scheduler->realtime_tasks.push_back(task);
		scheduler->num_realtime++;
	} else {
		scheduler->cfs_tasks.insert(task);
		scheduler->num_cfs++;
	}
}

static KTHREAD *select_new_task(KSCHEDULER *scheduler)
{
	if (!scheduler->realtime_tasks.empty()) {
		scheduler->num_realtime--;
		return scheduler->realtime_tasks.pop_front();
	}

	KTHREAD *thread = scheduler->cfs_tasks.first();
	if (thread) {
		scheduler->num_cfs--;
		scheduler->cfs_tasks.remove(thread);
	}

	return thread;
}

static void handle_preempt_timer(KTIMER *timer)
{
	KSCHEDULER *scheduler = container_of(&KSCHEDULER::preempt_timer, timer);
	scheduler->scheduler_lock.lock();
	scheduler->current_deadline = NSEC_MAX;

	if (!scheduler->next_thread) {
		KTHREAD *thread = select_new_task(scheduler);
		BUG_ON(!thread);

		scheduler->next_thread = thread;
		KeSetPendingPreemption();
	}

	scheduler->scheduler_lock.unlock();
}

static void schedule(KSCHEDULER *scheduler)
{
	KTHREAD *current = HalCurrentThread();

	/*
	 * Lock the scheduler.
	 */
	scheduler->scheduler_lock.lock();
	/*
	 * KeClearPendingPreemption must be called with DPCs masked (it is
	 * masked by the lock above).
	 */
	KeClearPendingPreemption();

	/*
	 * Read the next thread to run.  If we are preempted for any reason, the
	 * preempting thread will be stored in next_thread.  Otherwise, we do
	 * not reschedule.
	 */
	KTHREAD *next = scheduler->next_thread;
	scheduler->next_thread = nullptr;

	if (!next) {
		/*
		 * next_thread was nullptr.  Look at current thread state, see
		 * if we need to reschedule.
		 *
		 * If the current task is blocking, we need to reschedule.
		 * Otherwise don't, as it is not necessary.  This behavior means
		 * that another CPU can force an RCU quiescent state earlier by
		 * issuing a preemption-IPI to other CPUs without setting
		 * next_thread, without forcing a _real_ task switch.
		 */
		if (current->thread_state == KTHREAD_RUNNING)
			return;

		next = select_new_task(scheduler);
		/*
		 * If there is no next task but we must do a task switch, we
		 * must switch to the idle task.
		 */
		if (!next) {
			next = scheduler->idle_task;
			/*
			 * The idle task mustn't block.
			 */
			BUG_ON(current == next);
		}
	}

	BUG_ON(next == nullptr);

	int prio = current->current_priority;
	nsec_t now = HalReadSchedClock();

	/*
	 * Set last_vruntime_update of the task we are switching to.
	 */
	next->last_vruntime_update = now;

	/*
	 * If we are switching away from the idle task, we can skip a lot of
	 * work.
	 */
	if (current != scheduler->idle_task) {
		/*
		 * Account for spent vruntime if the current thread's priority
		 * is in the CFS scheduling range.
		 */
		if (prio <= KPRIORITY_MAX_CFS) {
			nsec_t elapsed_ns = now - current->last_vruntime_update;
			current->vruntime += vruntime_scale[prio] * elapsed_ns;
		}
		if (current->thread_state == KTHREAD_RUNNING) {
			insert_task(scheduler, current, prio);
		}
	}

	KTHREAD *previous = context_switch(current, next);
	KiFinalizeTaskSwitch(previous);
}

/**
 * KeReschedule - switch to another task if necessary.
 *
 * KeReschedule() doesn't necessarily reschedule immediately.  KeReschedule will
 * always reschedule if one of the following conditions are met:
 *
 * 1) KSCHEDULER::next_thread is non-NULL.
 *
 * 2) The current KTHREAD's thread_state is not KTHREAD_RUNNING.
 *
 * i.e., if a thread is preempting us or we are blocking, we reschedule.
 */
void KeReschedule(void)
{
	do {
		KeDisablePreemption();
		schedule(HalPtrThisCpu(kiScheduler));
	} while (KiEnablePreemptionAndTest());
}

/**
 * KiInitializeScheduler - initialize the CPU scheduler.
 */
void KiInitializeScheduler(void)
{
	for (unsigned int i = 0; i < keProcessorCount; i++) {
		KSCHEDULER *scheduler = HalPtrPerCPU(kiScheduler, i);

		ETHREAD *ethread = ExCreateIdleThread(i);

		scheduler->current_thread = &ethread->thread;
		scheduler->idle_task = &ethread->thread;

		if (!i)
			HalSetCurrentThread(&ethread->thread);
	}
}

/**
 * KiInitializeSchedulerOnSecondaryProcessor - start the scheduler on a
 * secondary processor.
 */
void KiInitializeSchedulerOnSecondaryProcessor(void)
{
	KSCHEDULER *scheduler = HalPtrThisCpu(kiScheduler);

	HalSetCurrentThread(scheduler->idle_task);
}

void KeSetCurrentState(int state)
{
	KTHREAD *thread = HalCurrentThread();
	atomic_store(&thread->thread_state, state, _MO_SeqCst);
}

