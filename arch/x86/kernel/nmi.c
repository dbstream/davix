/**
 * Machine-check errors and non-maskable interrupts (NMI).
 * Copyright (C) 2024  dbstream
 */
#include <davix/panic.h>
#include <davix/printk.h>
#include <asm/entry.h>
#include <asm/smp.h>

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
	if (check_panic ())
		return;

	printk (PR_INFO "Received non-maskable interrupt on CPU%u\n",
		this_cpu_id ());
}

void
handle_MCE (struct entry_regs *regs)
{
	(void) regs;
	if (check_panic ())
		return;

	panic ("Machine-Check Error");
}
