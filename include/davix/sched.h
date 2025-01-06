/**
 * Scheduler functions.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_SCHED_H
#define _DAVIX_SCHED_H 1

#include <davix/sched_types.h>

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
