/**
 * Creation of new processes.
 * Copyright (C) 2024  dbstream
 */
#include <davix/panic.h>
#include <davix/sched.h>
#include <davix/slab.h>
#include <davix/snprintf.h>
#include <davix/task_api.h>

static struct slab *tasks_slab;

void
tasks_init (void)
{
	tasks_slab = slab_create ("struct task", sizeof (struct task));
	if (!tasks_slab)
		panic ("Failed to create struct task allocator");
}

struct task *
alloc_task_struct (void)
{
	return slab_alloc (tasks_slab);
}

void
free_task_struct (struct task *task)
{
	slab_free (task);
}

struct task *
create_idle_task (unsigned int cpu)
{
	struct task *task = alloc_task_struct ();
	if (!task)
		panic ("Failed to create idle task for CPU%u\n", cpu);

	snprintf (task->comm, sizeof (task->comm), "idle-%u", cpu);

	task->flags = TF_IDLE;
	return task;
}

struct task *
create_kernel_task (const char *name, void (*start_function)(void *), void *arg)
{
	struct task *task = alloc_task_struct ();
	if (!task)
		return NULL;

	if (arch_create_task (task, start_function, arg) != ESUCCESS) {
		free_task_struct (task);
		return NULL;
	}

	snprintf (task->comm, sizeof (task->comm), "%s", name);

	task->flags = 0;
	task->state = TASK_RUNNABLE;
	sched_new_task (task);
	return task;
}