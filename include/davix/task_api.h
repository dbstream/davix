/**
 * Task creation, etc...
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_TASK_API_H
#define _DAVIX_TASK_API_H 1

struct task;

extern void
tasks_init (void);

extern struct task *
alloc_task_struct (void);

extern void
free_task_struct (struct task *task);

extern struct task *
create_idle_task (unsigned int cpu);

/**
 * Create a new task which will execute in kernel context.
 * @flags	bitmask of TF_* values.
 *	TF_NOMIGRATE	The task will run on the current CPU.
 */
extern struct task *
create_kernel_task (const char *name, void (*start_function) (void *), void *arg,
	unsigned int flags);

#endif /* _DAVIX_TASK_API_H */
