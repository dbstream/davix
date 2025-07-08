/**
 * Kernel thread helper.
 * Copyright (C) 2025-present  dbstream.
 */
#include <asm/irql.h>
#include <asm/task.h>
#include <davix/kthread.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/task.h>
#include <string.h>

Task *
kthread_create (const char *name, void (*function)(void *), void *arg)
{
	if (!function)
		printk (PR_ERROR "kthread_create with NULL function!\n");
	Task *task = alloc_task_struct ();
	if (!task)
		return nullptr;

	init_task_struct_fields (task);
	strncpy (task->comm, name, sizeof (task->comm));
	task->comm[sizeof (task->comm) - 1] = '\0';

	if (!arch_create_task (task, function, arg)) {
		free_task_struct (task);
		return nullptr;
	}

	task->ctx_fs = fsctx_get (&init_fs_context);
	return task;
}

void
kthread_start (Task *task)
{
	if (!sched_wake (task, SCHED_WAKE_INITIAL))
		printk (PR_WARN "kthread_start: sched_wake returned false\n");
}

void
kthread_exit (void)
{
	disable_dpc ();
	set_current_state (TASK_ZOMBIE);
	schedule ();
}

void
reap_task (Task *tsk)
{
	/* TODO: properly handle non-kthread tasks  */
	fsctx_put (tsk->ctx_fs);
	arch_free_task (tsk);
	free_task_struct (tsk);
}
