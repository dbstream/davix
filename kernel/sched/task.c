/* SPDX-License-Identifier: MIT */
#include <davix/errno.h>
#include <davix/sched.h>
#include <davix/mm.h>
#include <davix/kmalloc.h>
#include <davix/printk.h>

void handle_dead_task(struct task *task)
{
	info("sched: Task \"%.*s\" (%p) died.\n", COMM_LEN, task->comm, task);

	if(drop(&task->mm->refcnt))
		destroy_mm(task->mm);

	arch_task_died(task);
	kfree(task);
}

int create_kernel_task(const char *name, void (*function)(void *), void *arg)
{
	struct task *task = kmalloc(sizeof(struct task));
	if(!task)
		return ENOMEM;

	strncpy(task->comm, name, COMM_LEN);

	task->task_state = TASK_RUNNING;
	task->flags = 0;
	task->mm = &kernelmode_mm;
	grab(&kernelmode_mm.refcnt);

	int err = arch_setup_task(task, function, arg);
	if(err) {
		kfree(task);
		return err;
	}

	sched_start_task(task);

	return 0;
}
