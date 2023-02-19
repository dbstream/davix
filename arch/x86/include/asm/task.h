/* SPDX-License-Identifier: MIT */
#ifndef __ASM_TASK_H
#define __ASM_TASK_H

#include <asm/backtrace.h>

/*
 * The stack frame pointed to by idle tasks.
 *
 * __switch_to will pop from this.
 */
struct idle_task_frame {
	/*
	 * These registers aren't callee save/restore per SysV ABI, but we
	 * save/restore them from stack anyways as the arguments to a function.
	 */
	unsigned long rdi, rsi;

	/*
	 * These are the registers we need to save/restore per SysV ABI.
	 */
	unsigned long r15, r14, r13, r12, rbx;
	struct stack_frame *rbp;

	unsigned long rip;

	/*
	 * This field is special, as it is not pushed/popped by __switch_to, but
	 * it should be zeroed when a new task is created, and rbp should be set
	 * to point to it.
	 *
	 * This is needed for backtracing to work.
	 */
	struct stack_frame null_frame;
};

/*
 * x86-specific task info, embedded into the struct task as the first field.
 */
struct arch_task_info {
	/*
	 * Keep this field first, so that it can be accessed as offset 0 from
	 * __switch_to.
	 */
	struct idle_task_frame *kernel_sp;

	/*
	 * Used by syscalls and fault handlers.
	 */
	struct page *kernel_stack;

	/*
	 * A separate timer interrupt stack for handling the timer interrupt.
	 *
	 * This task needs to be separate from the kernel stack, because on
	 * interrupted syscalls before the kernel stack is loaded, CPL is 0
	 * meaning that TSS::rsp0 is not loaded, therefore the interrupt uses a
	 * garbage stack pointer, which is bad.
	 */
	struct page *timer_stack;
};

#endif /* __ASM_TASK_H */
