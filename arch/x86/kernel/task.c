/**
 * Task initialization and finalization on x86.
 * Copyright (C) 2024  dbstream
 */
#include <davix/kmalloc.h>
#include <davix/sched.h>
#include <davix/stddef.h>
#include <asm/task.h>
#include <asm/trap.h>

extern void
__arch_switch_to (struct task *new_task, struct task *old_task);

void
arch_switch_to (struct task *new_task, struct task *old_task)
{
	x86_update_rsp0 ((unsigned long) new_task->arch.stack_mem + TASK_STK_SIZE);
	__arch_switch_to (new_task, old_task);
}

extern errno_t
arch_create_task (struct task *task, void (*start_function) (void *), void *arg)
{
	task->arch.stack_mem = kmalloc (TASK_STK_SIZE);
	if (!task->arch.stack_mem)
		return ENOMEM;

	task->arch.stack_ptr = (struct x86_idle_stack *)
		((unsigned long) task->arch.stack_mem + TASK_STK_SIZE
			- sizeof (struct x86_idle_stack) - 8);

	task->arch.stack_ptr->ip = (unsigned long) start_function;
	task->arch.stack_ptr->rdi = (unsigned long) arg;
	return 0;
}

extern void
arch_destroy_task (struct task *task)
{
	kfree (task->arch.stack_mem);
}
