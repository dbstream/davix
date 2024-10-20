/**
 * Scheduler internals.
 * Copyright (C) 2024  dbstream
 */
#ifndef _SCHED_INTERNAL_H
#define _SCHED_INTERNAL_H 1

#include <davix/list.h>
#include <davix/spinlock.h>
#include <davix/time.h>
#include <asm/cpulocal.h>

/**
 * There is exactly one runqueue per CPU. Each runqueue has its own idle task.
 */

struct task;

struct runqueue {
	struct task *idle_task;

	struct list runnable_list;

	nsecs_t last_task_switch;
};

extern struct runqueue sched_runqueues;

static inline struct runqueue *
this_cpu_runqueue (void)
{
	return this_cpu_ptr (&sched_runqueues);
}

static inline struct runqueue *
that_cpu_runqueue (unsigned int cpu)
{
	return that_cpu_ptr (&sched_runqueues, cpu);
}

/**
 *   Operations on runqueues
 *
 * The scheduling algorithm is kept separate from the main scheduler by
 * providing an interface that can enqueue, dequeue, select the next task,
 * and get a deadline for the currently-running task.
 *
 * note: all operations on runqueues must be performed with local interrupts
 * disabled. The caller of rq_* functions is responsible for ensuring this.
 */

/**
 * Initialize algorithm parts of a runqueue.
 */
extern void
rq_init (struct runqueue *rq);

/**
 * Insert a task into the runnable list. Returns true if the deadline of the
 * current timeslice changed as a result.
 */
extern bool
rq_enqueue (struct task *task);

/**
 * Remove a task from the runnable list. Returns true if the deadline of the
 * current timeslice changed as a result.
 */
extern bool
rq_dequeue (struct task *task);

/**
 * Select the next task to run, and pop it off the runqueue. If no task is
 * available for execution, this function returns NULL.
 */
extern struct task *
rq_choose_next (void);

/**
 * Get the deadline of the current timeslice, or zero if indefinite (eg. RT).
 */
extern nsecs_t
rq_get_deadline (void);

#endif /* _SCHED_INTERNAL_H */
