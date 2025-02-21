/**
 * Machine-check errors and non-maskable interrupts (NMI).
 * Copyright (C) 2024  dbstream
 */
#include <davix/context.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <asm/entry.h>
#include <asm/smp.h>
#include <asm/unwind.h>

/**
 * If we are in panic, halt the CPU. panic_nmi_handler only returns if the
 * current CPU is the panicking CPU, in which case we effectively make the NMI
 * and MCE handlers no-ops.
 */
static int
check_panic (void)
{
	if (in_panic ()) {
		panic_nmi_handler ();
		return 1;
	}
	return 0;
}

void
handle_NMI (struct entry_regs *regs)
{
	(void) regs;

	preempt_state_t state = preempt_enter_NMI ();

	if (!check_panic ())
		printk (PR_INFO "Received non-maskable interrupt on CPU%u\n",
			this_cpu_id ());

	dump_backtrace ();

	preempt_leave_NMI (state);
}

void
handle_MCE (struct entry_regs *regs)
{
	(void) regs;

	preempt_state_t state = preempt_enter_IRQ ();
	if (!check_panic ())
		panic ("Machine-Check Error");
	preempt_leave_IRQ (state);
}
