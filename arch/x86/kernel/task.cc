/**
 * Task state management.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/asm.h>
#include <asm/entry.h>
#include <asm/gdt.h>
#include <asm/irql.h>
#include <asm/percpu.h>
#include <davix/panic.h>
#include <davix/sched.h>
#include <davix/task.h>
#include <davix/vmap.h>
#include <string.h>
#include <stdint.h>

struct task_switch_frame {
	uint64_t rbp;
	uint64_t rbx;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t ip;

	/*
	 * This is padding which is used by PUSH_ENTRY_REGS and POP_ENTRY_REGS
	 * to ensure that the stack pointer is aligned.
	 */
	uint64_t padding;
	entry_regs initial_eregs;
};

struct gdt_struct {
	uint64_t entries[GDT_NUM_ENTRIES];
};

extern "C" DEFINE_PERCPU(gdt_struct, percpu_GDT);

PERCPU_CONSTRUCTOR(init_gdt)
{
	gdt_struct *source = percpu_ptr(percpu_GDT);
	gdt_struct *target = percpu_ptr(percpu_GDT).on (cpu);

	memcpy (target, source, sizeof (gdt_struct));
}

extern "C"
Task *
asm_switch_to (Task *me, Task *next);

Task *
arch_context_switch (Task *me, Task *next)
{
	entry_regs *old_eregs = get_user_entry_regs ();

	/*
	 * We must save and restore DPC and IRQ state across context switches.
	 * This is easiest to do by temporarily disabling raw IRQs.
	 */
	raw_irq_disable ();

	irql_t dpc_count = __read_irql_dispatch () & ~__IRQL_NONE_PENDING;
	irql_t irq_count = __read_irql_high () & ~__IRQL_NONE_PENDING;

	set_current_task (next);
	Task *prev = asm_switch_to (me, next);

	/*
	 * Restore DPC and IRQ state.  Note that we only restore our own counts
	 * here, leaving __IRQL_NONE_PENDING as is.
	 */
	irql_t irq_pending = __read_irql_high () & __IRQL_NONE_PENDING;
	irql_t dpc_pending = __read_irql_dispatch () & __IRQL_NONE_PENDING;

	__write_irql_high (irq_pending | irq_count);
	__write_irql_dispatch (dpc_pending | dpc_count);

	set_user_entry_regs (old_eregs);

	if (irq_pending == __IRQL_NONE_PENDING)
		/*
		 * When a task returns to us, we effectively inherit the
		 * __IRQL_NONE_PENDING flag from when it ran on this CPU (it
		 * may have changed).  That's why we don't use raw_irq_save.
		 */
		raw_irq_enable ();

	return prev;
}

extern "C"
void
asm_ret_from_new_task (void);

extern "C"
void
arch_ret_from_new_task (void *arg, Task *prev, void (*entry_function)(void *))
{
	/*
	 * We just got switched to from another task.  We are currently running
	 * with interrupts disabled: setup the initial DPC and IRQ state and
	 * reenable hardware interrupts.
	 */
	irql_t irq_pending = __read_irql_high () & __IRQL_NONE_PENDING;
	irql_t dpc_pending = __read_irql_dispatch () & __IRQL_NONE_PENDING;

	/*
	 * We initially set the IRQ and DPC disabling counts to one.
	 */
	__write_irql_high (irq_pending | 1);
	__write_irql_dispatch (dpc_pending | 1);

	if (irq_pending == __IRQL_NONE_PENDING)
		/*
		 * When we start, we inherit the __IRQL_NONE_PENDING flag from
		 * the previous task that ran on this CPU.  This means we must
		 * check if there is a pending lazy IRQ which has masked raw
		 * IRQs.
		 */
		raw_irq_enable ();

	/*
	 * Finish the context switch.  We must do this with IRQs and DPCs
	 * disabled.
	 */
	finish_context_switch (prev);

	/*
	 * Now we enable IRQs and DPCs before calling the entry function.
	 */
	enable_irq ();
	enable_dpc ();
	entry_function (arg);

	/*
	 * TODO: return to userspace instead.
	 */
	panic ("arch_ret_from_new_task: entry_function returned");
}

bool
arch_create_task (Task *task, void (*entry_function)(void *), void *arg)
{
	void *stack_bottom = kmalloc_large (0x4000);
	if (!stack_bottom)
		return false;

	task_switch_frame *initial_frame = (task_switch_frame *)
		((uintptr_t) stack_bottom + 0x4000 - sizeof(task_switch_frame));

	memset (initial_frame, 0, sizeof (*initial_frame));
	initial_frame->ip = (uintptr_t) asm_ret_from_new_task;
	initial_frame->r12 = (uintptr_t) entry_function;
	initial_frame->r13 = (uintptr_t) arg;

	task->arch.stack_bottom = stack_bottom;
	task->arch.stack_pointer = initial_frame;
	return true;
}

void
arch_free_task (Task *task)
{
	/*
	 * FIXME: actually freeing task data causes faults.  Most likely because
	 * of incorrect page table manipulation in vmap.
	 */
	if (1)
		return;

	kfree_large (task->arch.stack_bottom);
}
