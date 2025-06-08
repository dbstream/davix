/**
 * Low-level IRQ entry code.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/apic.h>
#include <asm/entry.h>
#include <asm/interrupt.h>
#include <asm/irql.h>
#include <asm/smp.h>
#include <davix/ktimer.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/smp.h>

static void
x86_handle_irq_vector (int irq);

extern "C" void
__entry_from_irq_vector (entry_regs *regs)
{
	set_user_entry_regs (regs);
	int irq = regs->error_code;

	irql_begin_irq_from_user ();
	x86_handle_irq_vector (irq);
	irql_leave_irq ();
}

extern "C" void
__entry_from_irq_vector_k (entry_regs *regs)
{
	int irq = regs->error_code;

	if (!irql_begin_irq_from_kernel (irq)) {
		/** disable interrupts when returning  */
		regs->rflags &= ~(1UL << 9);
		return;
	}

	x86_handle_irq_vector (irq);
	irql_leave_irq ();
}

void
x86_do_deferred_irq_vector (int irq)
{
	x86_handle_irq_vector (irq);
}

static void
x86_handle_irq_vector (int irq)
{
	if (irq == VECTOR_SPURIOUS) {
		printk (PR_INFO "IRQ: got spurious interrupt\n");
		return;
	}

	if (irq == VECTOR_APIC_TIMER) {
		apic_eoi ();
		ktimer_handle_timer_interrupt ();
		return;
	}

	if (irq == VECTOR_SMP_CALL_ON_ONE) {
		apic_eoi ();
		smp_handle_call_on_one_ipi ();
		return;
	}

	if (irq == VECTOR_SMP_PANIC) {
		apic_eoi ();
		panic ("CPU%u got SMP PANIC IPI\n", this_cpu_id ());
	}

	printk (PR_INFO "IRQ: got interrupt %d\n", irq);
}
