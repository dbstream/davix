/**
 * CPU masking of IRQs.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_IRQ_H
#define _ASM_IRQ_H 1

#include <davix/stdbool.h>

/**
 * Test if IRQs are enabled on this CPU.
 */
static inline bool
irqs_enabled (void)
{
	unsigned long rflags;
	/* NB: we compile with -mno-red-zone, so this is safe */
	asm ("pushfq; pop %0" : "=r" (rflags));

	return (rflags & (1UL << 9)) ? true : false;
}

/**
 * Disable acceptance of IRQs directed to this CPU.
 */
static inline void
irq_disable (void)
{
	asm volatile ("cli" ::: "memory");
}

/**
 * Enable acceptance of IRQs directed to this CPU.
 */
static inline void
irq_enable (void)
{
	asm volatile ("sti" ::: "memory");
}

/**
 * Start a critical section with IRQs disabled. Pairs with irq_restore().
 */
static inline bool
irq_save (void)
{
	if (!irqs_enabled ())
		return false;
	irq_disable ();
	return true;
}

/**
 * End a critical section with IRQs disabled. Pairs with irq_save().
 * The boolean argument is the value returned from irq_save().
 */
static inline void
irq_restore (bool flag)
{
	if (flag)
		irq_enable ();
}

/**
 * Wait for an interrupt to occur on the processor. This function may return
 * spuriously; for example if NMI or MCE occurs, or if SMM does not persist
 * the core halt status.
 */
static inline void
arch_wfi (void)
{
	asm volatile ("hlt");
}

/**
 * Yield the CPU to other hyperthreads. Expands to the 'pause' instruction on
 * x86.
 */
static inline void
arch_relax (void)
{
	asm volatile ("pause");
}

#endif /* _ASM_IRQ_H */
