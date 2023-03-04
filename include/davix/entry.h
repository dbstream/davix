/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_ENTRY_H
#define __DAVIX_ENTRY_H

#include <davix/types.h>
#include <davix/smp.h>
#include <asm/irq.h>

/*
 * Reschedule if it is needed.
 * This is called on kernel->user return, IRQ return and
 * outermost preempt-enables.
 */
void maybe_resched(void);

extern unsigned long preempt_disabled cpulocal;

static inline void preempt_disable(void)
{
	bool irqs = interrupts_enabled();
	disable_interrupts();

	rdwr_cpulocal(preempt_disabled)++;

	if(irqs)
		enable_interrupts();
}

static inline void preempt_enable(void)
{
	rdwr_cpulocal(preempt_disabled)--;
	maybe_resched();
}

static inline bool preempt_enabled(void)
{
	bool irqs = interrupts_enabled();
	disable_interrupts();

	unsigned long value = rdwr_cpulocal(preempt_disabled);

	if(irqs)
		enable_interrupts();

	return value == 0;
}

#endif /* __DAVIX_ENTRY_H */
