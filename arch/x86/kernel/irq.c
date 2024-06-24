/**
 * APIC IRQ handlers.
 * Copyright (C) 2024  dbstream
 *
 * Here, we handle interrupts that originate from the local APIC.
 */
#include <davix/printk.h>
#include <asm/apic.h>
#include <asm/entry.h>
#include <asm/smp.h>

void
handle_IRQ (struct entry_regs *regs)
{
	int irq = regs->error_code;
	printk (PR_INFO "Received IRQ %d\n", irq);
}

void
handle_APIC_timer (struct entry_regs *regs)
{
	(void) regs;
	printk (PR_INFO "APIC timer interrupt on CPU%u\n", this_cpu_id ());
	apic_eoi ();
}

void
handle_IRQ_spurious (struct entry_regs *regs)
{
	(void) regs;
	printk (PR_WARN "Spurious interrupt on CPU%u\n", this_cpu_id ());
	apic_eoi ();
}
