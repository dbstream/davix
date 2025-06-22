/**
 * Davix scheduler.
 * Copyright (C) 2025-present  dbstream
 *
 * This is the "heart" of the Davix scheduler.
 */
#include <asm/percpu.h>
#include <asm/smp.h>
#include <asm/switch_to.h>
#include <davix/atomic.h>
#include <davix/cpuset.h>
#include <davix/dpc.h>
#include <davix/irql.h>
#include <davix/ktimer.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/rcu.h>
#include <davix/sched.h>
#include <davix/slab.h>
#include <davix/spinlock.h>
#include <davix/task.h>
#include <vsnprintf.h>

static constexpr nsecs_t FIXED_TIMESLICE_LENGTH = 5000000 /* 5ms */;

static void
sched_timer_fn (KTimer *tmr, void *arg)
{
	(void) tmr;
	(void) arg;

	set_pending_reschedule ();
}

typedef dsl::TypedList<Task, &Task::rq_list_entry> RQTaskList;

struct sched_runqueue {
	RQTaskList queues[MAX_TASK_PRIORITY - MIN_TASK_PRIORITY + 1];

	int current_priority;
	Task *current_task;
	Task *idle_task;

	spinlock_t rq_lock;

	/**
	 * The number of runnable tasks on this runqueue.  Used for load
	 * balancing purposes.  Read and written under rq_lock.
	 */
	unsigned int rq_load;
};

static DEFINE_PERCPU(sched_runqueue, runqueue);
static DEFINE_PERCPU(KTimer, sched_timer);

static DEFINE_PERCPU(RQTaskList, reap_list);
static DEFINE_PERCPU(DPC, reap_dpc);

/**
 * reap_dpc_func - reap tasks in a DPC.
 */
static void
reap_dpc_func (DPC *dpc, void *arg1, void *arg2)
{
	(void) dpc;
	(void) arg1;
	(void) arg2;

	RQTaskList *rl = percpu_ptr (reap_list);
	while (!rl->empty ())
		reap_task (rl->pop_front ());
}

PERCPU_CONSTRUCTOR(sched_pcpu)
{
	sched_runqueue *rq = percpu_ptr (runqueue).on (cpu);
	KTimer *tmr = percpu_ptr (sched_timer).on (cpu);
	RQTaskList *rl = percpu_ptr (reap_list).on (cpu);
	DPC *rd = percpu_ptr (reap_dpc).on (cpu);

	for (int prio = MIN_TASK_PRIORITY; prio <= MAX_TASK_PRIORITY; prio++) {
		int idx = prio - MIN_TASK_PRIORITY;
		rq->queues[idx].init ();
	}

	rq->current_priority = MIN_TASK_PRIORITY;
	rq->current_task = nullptr;
	rq->idle_task = nullptr;
	rq->rq_lock.init ();
	rq->rq_load = 0;

	tmr->init (sched_timer_fn, nullptr);

	rl->init ();
	rd->init (reap_dpc_func, nullptr, nullptr);
}

/**
 * TODO: use some kind of tree-like structure to make find_least_loaded_cpu and
 * find_most_loaded_cpu fast.
 */

/**
 * Find the least loaded processor in the system.
 */
static unsigned int
find_least_loaded_cpu (void)
{
	unsigned int best_cpu = 0;
	unsigned int best_cpu_load = -1U;

	for (unsigned int cpu : cpu_online) {
		sched_runqueue *rq = percpu_ptr (runqueue).on (cpu);
		rq->rq_lock.lock_irq ();
		if (rq->rq_load < best_cpu_load) {
			best_cpu_load = rq->rq_load;
			best_cpu = cpu;
		}
		rq->rq_lock.unlock_irq ();
	}

	return best_cpu;
}

/**
 * handle_reschedule_IPI - handle the remote reschedule IPI.
 */
void
handle_reschedule_IPI (void)
{
	set_pending_reschedule ();
}

/**
 * Force schedule() to be called on another processor.
 * @cpu: CPU number of remote processor
 */
static void
reschedule_remote_processor (unsigned int cpu)
{
	arch_send_reschedule_IPI (cpu);
}

/**
 * Add @task to the runqueue of another processor.
 * @task: task to make runnable
 * @cpu: the CPU on which to enqueue the task
 */
static void
enqueue_on_remote_processor (Task *task, unsigned int cpu)
{
	sched_runqueue *rq = percpu_ptr (runqueue).on (cpu);

	int idx = task->current_priority - MIN_TASK_PRIORITY;

	disable_irq ();
	rq->rq_lock.raw_lock ();
	rq->rq_load++;
	task->task_state = TASK_RUNNABLE;
	bool was_empty = rq->queues[idx].empty ();
	rq->queues[idx].push_back (task);

	if (task->current_priority > rq->current_priority)
		/*
		 * If the task we are enqueueing is of a greater priority than
		 * the currently-running task on the runqueue, reschedule via
		 * IPI.
		 */
		reschedule_remote_processor (cpu);
	else if (task->current_priority == rq->current_priority && was_empty)
		/*
		 * If the runqueue was empty, there is no enqueued ktimer on the
		 * remote CPU.  Therefore we must send a reschedule IPI to make
		 * it aware of the task we just added to its runqueue.  Ideally,
		 * we want to avoid this, because the reschedule IPI causes an
		 * immediate reschedule when we really only needed to arm the
		 * scheduler timer.
		 */
		reschedule_remote_processor (cpu);

	rq->rq_lock.raw_unlock ();
	enable_irq ();
}

/**
 * Add @task to the runqueue of this processor.
 * @task: task to make runnable
 */
static void
enqueue_on_this_processor (Task *task)
{
	int idx = task->current_priority - MIN_TASK_PRIORITY;
	sched_runqueue *rq = percpu_ptr (runqueue);

	disable_irq ();
	rq->rq_lock.raw_lock ();
	rq->rq_load++;
	task->task_state = TASK_RUNNABLE;
	bool was_empty = rq->queues[idx].empty ();
	rq->queues[idx].push_back (task);

	if (task->current_priority > rq->current_priority)
		/*
		 * We are enqueueing a task of greater priority than the
		 * currently-running task:  reschedule as soon as possible.
		 */
		set_pending_reschedule ();
	else if (task->current_priority == rq->current_priority && was_empty) {
		/*
		 * If the runqueue was empty, there is no enqueued ktimer on
		 * this CPU: arm it.
		 */
		KTimer *tmr = percpu_ptr (sched_timer);
		tmr->enqueue (ns_since_boot () + FIXED_TIMESLICE_LENGTH);
	}
	rq->rq_lock.raw_unlock ();
	enable_irq ();
}

/*
 * Synchronization between __sched_wake() and schedule()
 *
 * Consider the following scenario:
 *	CPU1				CPU2
 *	T1				T2
 *	mutex_lock()
 *	->sched_get_blocking_ticket()
 *	  add_mutex_waiter(mutex, waiter)
 *	  mutex->lock.unlock()
 *					mutex_unlock()
 *					->mutex->lock.lock()
 *	  schedule()			  sched_wake(T1, ticket)
 *
 * It is possible to sched_wake a task before it has scheduled away.
 *
 * Our solution to this problem is to set a field in the task to be woken,
 * indicating to finish_context_switch() that there is a pending wakeup on
 * the task.  finish_context_switch() sets on_cpu to -1U before looking at
 * pending_wakeup, so if __sched_wake observes on_cpu != -1U after storing
 * 1 to pending_wakeup, it can return without doing anything.  Otherwise,
 * __sched_wake and finish_context_switch both perform an atomic CAS on
 * pending_wakeup, and only one of them "wins" and gets to wake the task up.
 */
/**
 * __sched_wake - wake up the task when we know we are the only possible waker.
 * @task: task to wake up
 */
static void
__sched_wake (Task *task)
{
	atomic_store (&task->pending_wakeup, 1, mo_seq_cst);
	/* happens-before */
	if (atomic_load (&task->on_cpu, mo_seq_cst) != -1U)
		/*
		 * If we see on_cpu != -1, we know that finish_context_switch
		 * hasn't looked at pending_wakeup yet.  Thus, it is safe to
		 * return here, because finish_context_switch will see our
		 * store to pending_wakeup.
		 *
		 * Returning here also means that we avoid enqueueing the task
		 * on a runqueue before it has completely scheduled away.
		 */
		return;

	int expected = 1;
	/*
	 * This compare-and-swap can use relaxed memory ordering, because we
	 * have already observed on_cpu == -1U set by finish_context_switch,
	 * which means that this happens-after the task has completely switched
	 * away.
	 */
	bool ret = atomic_cmpxchg (&task->pending_wakeup, &expected, 0,
			mo_relaxed, mo_relaxed);

	if (!ret)
		/*
		 * We observed pending_wakeup == 0.  This means enqueueing will
		 * happen in finish_context_switch.
		 */
		return;

	unsigned int this_cpu = this_cpu_id ();
	unsigned int prev_cpu = task->last_cpu;
	if (task->task_flags & TF_NOMIGRATE) {
		/*
		 * An unmigratable task must be enqueued on the CPU it last ran
		 * on.
		 */
		if (this_cpu == prev_cpu)
			enqueue_on_this_processor (task);
		else
			enqueue_on_remote_processor (task, prev_cpu);

		return;
	}

	/*
	 * Attempt to enqueue the task on the least loaded CPU in the system.
	 */
	unsigned int target = find_least_loaded_cpu ();
	task->last_cpu = target;
	if (this_cpu == target)
		enqueue_on_this_processor (task);
	else
		enqueue_on_remote_processor (task, target);
}

/**
 * finish_context_switch - finish context switching away from a task.
 * @prev: the task we just context switched away from
 */
void
finish_context_switch (Task *prev)
{
	int state = atomic_load_relaxed (&prev->task_state);

	if (state == TASK_ZOMBIE) {
		/*
		 * NB: we cannot call reap_task with interrupts disabled, as the
		 * architecture may use kfree_large to free the kernel stack at
		 * reap_task time.  Therefore we must reap tasks in a DPC.
		 */
		RQTaskList *rl = percpu_ptr (reap_list);
		DPC *dpc = percpu_ptr (reap_dpc);

		rl->push_back (prev);
		dpc->enqueue ();
		return;
	}

	if (state == TASK_RUNNABLE) {
		/*
		 * The task is in our runqueue, but we need to set on_cpu = -1U
		 * anyways to signal that the task can be stolen from it by an
		 * idle CPU.
		 *
		 * We can use release ordering here, because we don't need to
		 * look at the pending_wakeup field here.
		 */
		atomic_store (&prev->on_cpu, -1U, mo_release);
		return;
	}

	/*
	 * We have rescheduled away from @prev.  Now we must check if there is a
	 * pending wakeup signalled on it.
	 */
	int expected = 1;
	atomic_store (&prev->on_cpu, -1U, mo_seq_cst);
	/* happens-before */
	bool ret = atomic_cmpxchg (&prev->pending_wakeup, &expected, 0,
			mo_seq_cst, mo_seq_cst);

	if (ret) {
		/*
		 * There was a pending wakeup signalled on the task, and now we
		 * are the waker.
		 */
		if (prev->task_flags & TF_NOMIGRATE)
			/*
			 * An unmigratable task must be enqueued on the CPU it
			 * last ran on, which is this CPU.
			 */
			enqueue_on_this_processor (prev);
		else {
			/*
			 * Attempt to enqueue the task on the least loaded CPU
			 * in the system.
			 */
			unsigned int this_cpu = this_cpu_id ();
			unsigned int target = find_least_loaded_cpu ();
			prev->last_cpu = target;
			if (this_cpu == target)
				enqueue_on_this_processor (prev);
			else
				enqueue_on_remote_processor (prev, target);
		}
	}
}

/**
 * context_switch - perform a context switch between two tasks.
 */
static void
context_switch (Task *me, Task *next, sched_runqueue *rq)
{
	rq->current_priority = next->current_priority;
	/*
	 * Set on_cpu to this CPU.
	 */
	atomic_store_relaxed (&next->on_cpu, this_cpu_id ());

	/*
	 * If the runqueue at the current priority is not empty, reschedule in a
	 * round-robin fashion.
	 */
	if (!rq->queues[rq->current_priority].empty ()) {
		KTimer *tmr = percpu_ptr (sched_timer);

		tmr->enqueue (ns_since_boot () + FIXED_TIMESLICE_LENGTH);
	}

	rq->current_task = next;
	rq->rq_lock.raw_unlock ();
	Task *prev = arch_context_switch (me, next);
	finish_context_switch (prev);
}

/**
 * pick_next_task - choose the next task to run.
 *
 * This function must be called with the runqueue lock held.
 */
static Task *
pick_next_task (sched_runqueue *rq)
{
	/*
	 * Look for the highest-priority thread which is runnable.
	 */
	for (int prio = MAX_TASK_PRIORITY; prio >= MIN_TASK_PRIORITY; prio--) {
		int idx = prio - MIN_TASK_PRIORITY;
		if (!rq->queues[idx].empty ())
			return rq->queues[idx].pop_front ();
	}

	/*
	 * Pick the idle task if no task was runnable.
	 *
	 * TODO: pull tasks from the CPU which is the most loaded
	 */
	return rq->idle_task;
}

/**
 * enqueue_task - insert a task into the runqueue.
 *
 * This function must be called with the runqueue lock held.
 */
static void
enqueue_task (sched_runqueue *rq, Task *task)
{
	rq->queues[task->current_priority].push_back (task);
}

/**
 * schedule - perform a task switch.
 */
void
schedule (void)
{
	Task *me = get_current_task ();

	disable_dpc ();
	rcu_quiesce ();
	clear_pending_reschedule ();

	sched_runqueue *rq = percpu_ptr (runqueue);

	disable_irq ();
	rq->rq_lock.raw_lock ();
	if (me->task_state == TASK_RUNNABLE && !(me->task_flags & TF_IDLE)) {
		/*
		 * Reinsert ourselves into the runqueue if we are runnable.  We
		 * are runnable if we are not the idle task.
		 */
		enqueue_task (rq, me);
	} else if (!(me->task_flags & TF_IDLE))
		rq->rq_load--;

	Task *next = pick_next_task (rq);

	if (me != next)
		/*
		 * Perform the context switch only if we really are switching
		 * tasks.
		 *
		 * (context_switch will unlock rq_lock)
		 */
		context_switch (me, next, rq);
	else
		rq->rq_lock.raw_unlock ();

	enable_irq ();
	enable_dpc ();
}

/**
 * set_current_state - set the current task state.
 * @state: task state to set, must be one of TASK_*
 */
void
set_current_state (int state)
{
	atomic_store_release (&get_current_task ()->task_state, state);
}

/**
 * sched_get_blocking_ticket - acquire a blocking ticket for the current task.
 *
 * Returns a sched_ticket_t which must be passed to sched_wake to wake up the
 * task.  A call to sched_get_blocking ticket invalidates any previous tickets
 * returned such that the task can only be woken up with the ticket from the
 * most recent call.
 */
sched_ticket_t
sched_get_blocking_ticket (void)
{
	Task *me = get_current_task ();

	return atomic_inc_fetch (&me->unblock_ticket, mo_relaxed);
}

/**
 * sched_wake - wake up a task.
 * @task: the task to wake up.
 * @ticket: a ticket from sched_get_blocking_ticket or zero for initial wakeup.
 *
 * Returns true if we woke up the task.  Returns false if someone else woke up
 * the task before us, or if the blocking ticket changed.
 */
bool
sched_wake (Task *task, sched_ticket_t ticket)
{
	scoped_dpc g;
	bool ret = atomic_cmpxchg (&task->unblock_ticket, &ticket, ticket + 1,
			mo_relaxed, mo_relaxed);

	if (!ret)
		return false;

	__sched_wake (task);

	return true;
}

static SlabAllocator *task_allocator;

/**
 * sched_init - initialize the scheduler.
 */
void
sched_init (void)
{
	task_allocator = slab_create ("Task", sizeof (Task), alignof (Task));
	if (!task_allocator)
		panic ("Failed to create struct Task allocator!");

	/*
	 * setup the idle tasks on each CPU.
	 */
	for (unsigned int cpu : cpu_present) {
		Task *tsk = alloc_task_struct ();
		if (!tsk)
			panic ("Failed to allocate idle task for CPU%u!", cpu);

		tsk->task_state = TASK_RUNNABLE;
		tsk->task_flags = TF_IDLE | TF_NOMIGRATE;
		/*
		 * The idle tasks can be of a priority below MIN_TASK_PRIORITY,
		 * because they are never actually inserted into a CPU runqueue.
		 */
		tsk->base_priority = MIN_TASK_PRIORITY - 1;
		tsk->current_priority = MIN_TASK_PRIORITY - 1;
		tsk->unblock_ticket = SCHED_WAKE_INITIAL;
		tsk->pending_wakeup = 0;
		tsk->on_cpu = cpu;
		tsk->last_cpu = cpu;

		snprintf (tsk->comm, sizeof(tsk->comm), "idle-%u", cpu);

		sched_runqueue *rq = percpu_ptr (runqueue).on (cpu);
		rq->idle_task = tsk;
		rq->current_task = tsk;

		if (cpu == this_cpu_id ())
			set_current_task (tsk);
	}
}

/**
 * sched_init_this_cpu - initialize the scheduler on a freshly-booted CPU.
 */
void
sched_init_this_cpu (void)
{
	sched_runqueue *rq = percpu_ptr (runqueue);
	set_current_task (rq->idle_task);
	rcu_enable ();
}

Task *
alloc_task_struct (void)
{
	return (Task *) slab_alloc (task_allocator, ALLOC_KERNEL);
}

void
free_task_struct (Task *tsk)
{
	slab_free (tsk);
}

void
init_task_struct_fields (Task *tsk)
{
	tsk->task_state = TASK_RUNNABLE;
	tsk->task_flags = 0;
	tsk->base_priority = 10;
	tsk->current_priority = 10;
	tsk->unblock_ticket = SCHED_WAKE_INITIAL;
	tsk->pending_wakeup = 0;
	tsk->on_cpu = -1U;
	tsk->last_cpu = this_cpu_id ();
	tsk->comm[0] = '\0';
}

bool
has_pending_signal (void)
{
	return false;
}

