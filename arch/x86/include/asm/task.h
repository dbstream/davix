/**
 * Task initialization, switching, and finalization specific to x86.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_TASK_H
#define _ASM_TASK_H 1

#include <davix/errno.h>

/**
 * 16KiB ought to be enough for anyone in-kernel.
 */
#define TASK_STK_SIZE 0x4000

struct task;

static inline struct task *
get_current_task (void)
{
	struct task *tsk;
	asm volatile ("movq %%gs:16, %0" : "=r" (tsk));
	return tsk;
}

static inline void
set_current_task (struct task *tsk)
{
	asm volatile ("movq %0, %%gs:16" :: "r" (tsk));
}

/**
 * This is the stack layout in memory when a task is currently
 * not running.
 */
struct x86_idle_stack {
	unsigned long rbp;
	unsigned long rbx;
	unsigned long r12;
	unsigned long r13;
	unsigned long r14;
	unsigned long r15;

	unsigned long rdi;	/* arg */
	unsigned long ip;	/* start_function or caller of arch_switch_to */
};

/**
 * x86-specific task state.
 *
 * arch_task_state is guaranteed to be the first member (hence offset 0) of
 * struct task.
 */
struct arch_task_state {
	struct x86_idle_stack *stack_ptr;
	void *stack_mem;
};

/**
 * Initialize architecture-specific state for the task (e.g. stack) such that,
 * when switched to, start_function will be called with arg as the only
 * argument.
 */
extern errno_t
arch_create_task (struct task *task, void (*start_function)(void *), void *arg);

/**
 * Destroy architecture-specific state for the task (e.g. free the stack).
 */
extern void
arch_destroy_task (struct task *task);

/**
 * Switch the current execution context from old_task to new_task. This function
 * does not return unless something switches back to old_task.
 */
extern void
arch_switch_to (struct task *new_task, struct task *old_task);

#endif /* _ASM_TASK_H */
