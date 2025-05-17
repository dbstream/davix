/**
 * IRQL management.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/asm.h>
#include <asm/entry.h>
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
		x86_do_deferred_irq_vector (percpu_read (enqueued_vector));
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

void
irql_begin_irq_from_user (void)
{
	__raise_irql_dispatch ();
}

bool
irql_begin_irq_from_kernel (int irq)
{
	uint8_t irql = __read_irql_high () & ~__IRQL_NONE_PENDING;
	if (irql) {
		percpu_write (enqueued_vector, irq);
		__write_irql_high (irql);
		return false;
	}

	__raise_irql_dispatch ();
	return true;
}

void
irql_leave_irq (void)
{
	/**
	 * When dispatching DPCs, we must enable interrupts.  When we do this,
	 * we may be preempted by another interrupt.
	 *
	 * To limit DPC/IRQ recursion, we have two rules:
	 *   1. disable DPCs before enabling interrupts.
	 *   2. disable interrupts before enabling DPCs.
	 */

	while (__lower_irql_dispatch ()) {
		__write_irql_dispatch (1 | __IRQL_NONE_PENDING);
		raw_irq_enable ();
		dispatch_DPCs ();
		raw_irq_disable ();
	}
}
