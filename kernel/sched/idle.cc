/**
 * Davix scheduler
 * Copyright (C) 2025-present  dbstream
 *
 * This is the idle task.
 */
#include <asm/asm.h>
#include <asm/irql.h>
#include <davix/rcu.h>
#include <davix/sched.h>

static void
idle_wait (void)
{
	disable_dpc ();
	disable_irq ();

	rcu_disable ();
	if (has_pending_dpc ()) {
		rcu_enable ();
		enable_irq ();
		enable_dpc ();
		return;
	}

	raw_irq_disable ();
	if (has_pending_irq ()) {
		rcu_enable ();
		enable_irq ();
		enable_dpc ();
		return;
	}

	raw_irq_enable_wfi ();
	rcu_enable ();
	enable_irq ();
	enable_dpc ();
}

/**
 * sched_idle - become an idle task.
 */
void
sched_idle (void)
{
	for (;;) {
		schedule ();
		idle_wait ();
	}
}
