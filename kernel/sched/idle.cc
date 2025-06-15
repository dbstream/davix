/**
 * Davix scheduler
 * Copyright (C) 2025-present  dbstream
 *
 * This is the idle task.
 */
#include <asm/irql.h>
#include <davix/sched.h>

/**
 * sched_idle - become an idle task.
 */
void
sched_idle (void)
{
	for (;;) {
		schedule ();
		wait_for_interrupt ();
	}
}
