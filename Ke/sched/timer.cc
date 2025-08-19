// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/sched/timeout.cc
 * Timer management.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/smp.h>
#include <Ke/spinlock.h>
#include <Ke/systimer.h>
#include <Ke/timer.h>
#include <davix/bug.h>

/*
 * Because KTIMERs are per-CPU and cannot be dequeued on a remote processor, and
 * threads can migrate across processors during sleep, timeouts need a different
 * timer mechanism which can be dequeued on remote processors.  Such a facility
 * is implemented in this file under the name KSYSTIMER.
 */

struct SysTimerComparator {
	constexpr bool operator()(const KSYSTIMER *lhs, const KSYSTIMER *rhs) const
	{
		return lhs->expiry < rhs->expiry;
	}
};

typedef dsl::TypedAVLTree<KSYSTIMER, &KSYSTIMER::avl, SysTimerComparator> KSysTimerTree;

struct systimer_percpu {
	KTIMER ktimer;
	KSysTimerTree tree;
	nsec_t current_expiry;
	KeDPCSpinlock lock;
};

static DEFINE_PERCPU(systimer_percpu, systimer_percpu_data);

static void handle_timer_event(KTIMER *timer);

HAL_PERCPU_CALLBACK(cpu)
{
	systimer_percpu *pcpd = HalPtrPerCPU(systimer_percpu_data, cpu);
	pcpd->ktimer.init(handle_timer_event);
	pcpd->tree.init();
	pcpd->current_expiry = NSEC_MAX;
	pcpd->lock.init();
}

static void handle_timer_event(KTIMER *timer)
{
	systimer_percpu *pcpd = HalPtrThisCpu(systimer_percpu_data);

	nsec_t now = HalReadSchedClock();

	pcpd->lock.lock();
	pcpd->current_expiry = 0;
	for (;;) {
		if (pcpd->tree.empty()) {
			pcpd->current_expiry = NSEC_MAX;
			break;
		}

		KSYSTIMER *entry = pcpd->tree.first();
		if (entry->expiry > now)
			now = HalReadSchedClock();
		if (entry->expiry > now) {
			pcpd->current_expiry = entry->expiry;
			BUG_ON(!KeSetTimer(timer, pcpd->current_expiry));
			break;
		}

		KSYSTIMER_CALLBACK callback = entry->callback;
		pcpd->tree.remove(entry);
		atomic_store_release(&entry->on_queue, false);
		pcpd->lock.unlock();
		callback(entry);
		pcpd->lock.lock();
	}
	pcpd->lock.unlock();
}

/**
 * KeSetSysTimer - enqueue a KSYSTIMER.
 * @timer: KSYSTIMER to enqueue
 * @expiry: nanosecond timer deadline
 * Returns true if we enqueued the timer, false if the timer was already queued.
 */
bool KeSetSysTimer(KSYSTIMER *timer, nsec_t expiry)
{
	BUG_ON(expiry == NSEC_MAX); /* disallowed */

	KeDisableDPCs();
	if (atomic_load_acquire(&timer->on_queue)) {
		KeEnableDPCs();
		return false;
	}

	timer->cpu = HalCurrentProcessor();
	timer->expiry = expiry;
	systimer_percpu *pcpd = HalPtrThisCpu(systimer_percpu_data);
	pcpd->lock.lock();

	pcpd->tree.insert(timer);

	if (expiry < pcpd->current_expiry) {
		if (pcpd->current_expiry != NSEC_MAX)
			BUG_ON(!KeUnsetTimer(&pcpd->ktimer));
		pcpd->current_expiry = expiry;
		BUG_ON(!KeSetTimer(&pcpd->ktimer, expiry));
	}

	pcpd->lock.unlock();
	KeEnableDPCs();
	return true;
}

/**
 * KeUnsetSysTimer - remove a KSYSTIMER.
 * @timer: KSYSTIMER to remove
 * Returns true if we removed the timer, false if it was not on the queue.
 */
bool KeUnsetSysTimer(KSYSTIMER *timer)
{
	if (!atomic_load_relaxed(&timer->on_queue))
		return false;

	unsigned int cpu = timer->cpu;
	systimer_percpu *pcpd = HalPtrPerCPU(systimer_percpu_data, cpu);
	pcpd->lock.lock();

	if (!timer->on_queue) {
		pcpd->lock.unlock();
		return false;
	}

	pcpd->tree.remove(timer);
	timer->on_queue = false;

	pcpd->lock.unlock();
	return true;
}

