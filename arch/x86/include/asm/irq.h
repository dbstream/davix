#ifndef __ASM_IRQ_H
#define __ASM_IRQ_H

#include <asm/rflags.h>

static inline int interrupts_enabled(void)
{
	return x86_read_rflags() & __RFLAGS_IF ? 1 : 0;
}

static inline void disable_interrupts(void)
{
	asm volatile("cli");
}

static inline void enable_interrupts(void)
{
	asm volatile("sti");
}

#endif /* __ASM_IRQ_H */
