/**
 * APIC IRQ handlers.
 * Copyright (C) 2024  dbstream
 *
 * Here, we handle interrupts that originate from the local APIC.
 */
#include <davix/context.h>
#include <davix/printk.h>
#include <davix/timer.h>
#include <asm/apic.h>
#include <asm/entry.h>
#include <asm/smp.h>

void
handle_IRQ (struct entry_regs *regs)
{
	preempt_state_t state = preempt_enter_IRQ ();

	int irq = regs->error_code;
	printk (PR_INFO "Received IRQ %d\n", irq);
	apic_eoi ();

	preempt_leave_IRQ (state);
}

void
handle_APIC_timer (struct entry_regs *regs)
{
	preempt_state_t state = preempt_enter_IRQ ();

	(void) regs;
	printk (PR_INFO "APIC timer interrupt on CPU%u\n", this_cpu_id ());
	apic_eoi ();

	local_timer_tick ();

	preempt_leave_IRQ (state);
}

void
handle_IRQ_spurious (struct entry_regs *regs)
{
	preempt_state_t state = preempt_enter_IRQ ();

	(void) regs;
	printk (PR_WARN "Spurious interrupt on CPU%u\n", this_cpu_id ());
	apic_eoi ();

	preempt_leave_IRQ (state);
}

void
handle_PIC_IRQ (struct entry_regs *regs)
{
	preempt_state_t state = preempt_enter_IRQ ();

	(void) regs;
	printk (PR_WARN "Spurious 8259 PIC interrupt on CPU%u\n", this_cpu_id ());

	preempt_leave_IRQ (state);
}
