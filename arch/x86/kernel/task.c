/**
 * Task initialization and finalization on x86.
 * Copyright (C) 2024  dbstream
 */
#include <davix/sched.h>
#include <davix/stddef.h>
#include <davix/initmem.h>

extern errno_t
arch_create_task (struct task *task, void (*start_function) (void *), void *arg)
{
	if (mm_is_early)
		task->arch.stack_mem = initmem_alloc_virt (TASK_STK_SIZE, TASK_STK_SIZE);
	else
		task->arch.stack_mem = NULL; /* TODO: use page allocator */

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
	if (mm_is_early)
		initmem_free_virt (task->arch.stack_mem, TASK_STK_SIZE);

	/* TODO: use page allocator */
}
