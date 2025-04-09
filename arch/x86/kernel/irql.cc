/**
 * IRQL management.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/asm.h>
#include <asm/irql.h>
#include <asm/percpu.h>
#include <davix/dpc.h>
#include <davix/printk.h>

static DEFINE_PERCPU(int, enqueued_vector)

static inline void
handle_irq_vector (int vector)
{
	printk (PR_INFO "Got interrupt vector %d!\n", vector);
}

void
__pending_high (void)
{
	do {
		__write_irql_high (1 | __IRQL_NONE_PENDING);
		handle_irq_vector (percpu_read (enqueued_vector));
	} while (__lower_irql_high ());
	raw_irq_enable ();
}

void
__pending_dpcs (void)
{
	do {
		__write_irql_dispatch (1 | __IRQL_NONE_PENDING);
		dispatch_DPCs ();
	} while (__lower_irql_dispatch ());
}
