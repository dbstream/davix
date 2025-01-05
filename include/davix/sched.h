/**
 * Scheduler functions.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_SCHED_H
#define _DAVIX_SCHED_H 1

#include <davix/list.h>
#include <asm/task.h>

struct process_mm;

struct rq_entry {
	struct list list;
};

struct task {
	/**
	 * Architecture-specific task state; must be the first member.
	 */
	struct arch_task_state arch;

	/**
	 * Task flags. Immutable after task creation.
	 */
	unsigned int flags;

	/**
	 * Task state. This must only be accessed on the CPU that the task
	 * belongs to, either in IRQ context or when IRQs are disabled.
	 * Otherwise races can occur.
	 */
	unsigned int state;

	/**
	 * This is the processor that the task belongs to.
	 */
	unsigned int cpu;

	/**
	 * rq_entry: only used when the task is on the runqueue.
	 */
	struct rq_entry rq_entry;

	char comm[16];

	/**
	 * This is the memory manager instance of the task (or NULL for kernel
	 * tasks).
	 */
	struct process_mm *mm;
};

#define TASK_RUNNABLE 0
#define TASK_INTERRUPTIBLE 1
#define TASK_UNINTERRUPTIBLE 2
#define TASK_ZOMBIE 3

#define TF_IDLE 1U		/** The task is an idle task.  */
#define TF_NOMIGRATE 2U		/** The task is to be excluded form migration.  */

/**
 * Initialize the scheduler on boot.
 */
extern void
sched_init (void);

/**
 * Initialize the scheduler on the currently running CPU.
 *
 * ( note: this does not need to be called on the BSP,
 *   as it is initialized by the call to sched_init.   )
 */
extern void
sched_init_this_cpu (void);

/**
 * Turn the current thread into an idle task.
 */
extern void
sched_idle (void);

/**
 * Reschedule the current task.
 */
extern void
schedule (void);

/**
 * This function must be called at the start of any new task.
 */
extern void
sched_begin_task (void);

/**
 * Tell the scheduler about a new task. This makes the task run.
 */
extern void
sched_new_task (struct task *task);

/**
 * Wake up a task on the current CPU.
 * !!! This function may only be called on tasks that belong to the current CPU.
 * It must be called with preemption disabled.
 */
extern void
sched_wake__this_cpu (struct task *task);

/**
 * Wake up a task.
 */
extern void
sched_wake (struct task *task);

#endif /* _DAVIX_SCHED_H */
