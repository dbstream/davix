/**
 * Instructions that invalidate the TLB.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_TLB_H
#define _ASM_TLB_H 1

static inline void
__invlpg (void *mem)
{
	asm volatile ("invlpg (%0)" :: "r" (mem) : "memory");
}

#endif /* _ASM_TLB_H */
