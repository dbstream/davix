/**
 * The bulk of the scheduler.
 * Copyright (C) 2024  dbstream
 */
#include <davix/context.h>
#include <davix/cpuset.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/smp.h>
#include <davix/stddef.h>
#include <davix/task_api.h>
#include <davix/timer.h>
#include <asm/cpulocal.h>
#include <asm/irq.h>
#include <asm/smp.h>
#include "internal.h"

struct runqueue sched_runqueues __CPULOCAL;

static bool __scheduler_running __CPULOCAL;

static inline bool
scheduler_running (void)
{
	return this_cpu_read (&__scheduler_running);
}

/**
 *   Concurrency
 *
 * There is one simple rule we follow, and it is that anything that accesses
 * runqueue state must do so on the CPU that the runqueue belongs to, with
 * interrupts disabled.
 *
 * This keeps locking to a minimum in the scheduler, at the expense of
 * complexity of interprocessor wakeups and task migrations, and anything else
 * which might want to access runqueue state for a non-local CPU. Such accesses
 * can be done using interprocessor interrupts.
 */

struct scheduler_timer {
	struct ktimer ktimer;
	bool is_armed;
};

static void
sched_timer_callback (struct ktimer *tmr)
{
	struct scheduler_timer *stmr = (struct scheduler_timer *) tmr;
	stmr->is_armed = false;

	set_preempt_state (get_preempt_state () & ~PREEMPT_NO_RESCHED);
}

static struct scheduler_timer sched_timer __CPULOCAL;

static void
update_current_timeslice (void)
{
	struct scheduler_timer *tmr = this_cpu_ptr (&sched_timer);
	nsecs_t t = rq_get_deadline ();

	if (tmr->is_armed) {
		ktimer_remove (&tmr->ktimer);
		tmr->is_armed = false;
	}

	if (!t)
		return;

	tmr->ktimer.expiry = t;
	tmr->is_armed = true;
	ktimer_add (&tmr->ktimer);
}

static void
__sched_wake__this_cpu (struct task *task)
{
	if (task->state != TASK_RUNNABLE) {
		task->state = TASK_RUNNABLE;
		if (rq_enqueue (task))
			update_current_timeslice ();
	}
}

/**
 * Wake up a task on the current CPU.
 * !!! This function may only be called on tasks that belong to the current CPU.
 * It must be called with preemption disabled.
 */
void
sched_wake__this_cpu (struct task *task)
{
	bool flag = irq_save ();
	__sched_wake__this_cpu (task);
	irq_restore (flag);
}

static void
sched_wake_ipi (void *arg)
{
	__sched_wake__this_cpu ((struct task *) arg);
}

/**
 * Wake up a task on any CPU.
 */
void
sched_wake (struct task *task)
{
	preempt_off ();
	if (task->cpu == this_cpu_id ())
		sched_wake__this_cpu (task);
	else
		smp_call_on_cpu (task->cpu, sched_wake_ipi, task);
	preempt_on ();
}

/**
 * Bring up the scheduler on the current CPU.
 */
void
sched_init_this_cpu (void)
{
	struct runqueue *rq = this_cpu_runqueue ();
	struct scheduler_timer *tmr = this_cpu_ptr (&sched_timer);

	/* we are always called in a context which will later call sched_idle */
	set_current_task (rq->idle_task);

	tmr->ktimer.expiry = 0;
	tmr->ktimer.callback = sched_timer_callback;
	tmr->is_armed = false;

//	printk ("begin scheduling on CPU%u\n", this_cpu_id ());
	this_cpu_write (&__scheduler_running, true);

	update_current_timeslice ();
}

/**
 * We are called at the start of start_function. We are just context-switched
 * into by schedule(). IRQs are disabled.
 */
void
sched_begin_task (void)
{
	set_preempt_state (PREEMPT_NO_RESCHED);
	irq_enable ();
}

void
sched_new_task (struct task *task)
{
	bool flag = irq_save ();
	task->cpu = this_cpu_id ();
	if (task->state != TASK_RUNNABLE) {
		irq_restore (flag);
		return;
	}

	if (rq_enqueue (task))
		update_current_timeslice ();
	irq_restore (flag);
}

static void
context_switch (struct task *next, struct task *self)
{
	struct runqueue *rq = this_cpu_runqueue ();

//	printk (PR_INFO "context_switch: %s --> %s\n",
//		self->comm, next->comm);

	rq->last_task_switch = ns_since_boot ();
	update_current_timeslice ();

	set_current_task (next);
	arch_switch_to (next, self);
}

void
schedule (void)
{
	if (!scheduler_running ())
		return;

	bool flag = irq_save ();

	struct runqueue *rq = this_cpu_runqueue ();
	struct task *self = get_current_task ();

	if (self->state == TASK_RUNNABLE && !(self->flags & TF_IDLE))
		rq_enqueue (self);

	struct task *next = rq_choose_next ();
	if (!next)
		next = rq->idle_task;

	preempt_state_t preempt_state = get_preempt_state ();

	if (self != next)
		context_switch (next, self);

	set_preempt_state (preempt_state | PREEMPT_NO_RESCHED);
	irq_restore (flag);
}

/**
 * Become an idle task.
 *
 * This is called by two things; kernel main and SMP startup code.
 */
void
sched_idle (void)
{
	set_preempt_state (0);
	schedule ();
	irq_enable ();

	for (;;) {
		arch_wfi ();
		schedule ();
	}
}

/**
 * Initialize global state for the scheduler.
 */
void
sched_init (void)
{
	for_each_present_cpu (cpu) {
		struct runqueue *rq = that_cpu_runqueue (cpu);
		rq_init (rq);

		rq->idle_task = create_idle_task (cpu);
		rq->last_task_switch = 0;
	}

	sched_init_this_cpu ();
}
