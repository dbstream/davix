/* SPDX-License-Identifier: MIT */
#ifndef __ASM_TLB_H
#define __ASM_TLB_H

struct pgop;

static inline void invlpg(unsigned long addr)
{
	asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

void tlb_flush(struct pgop *op);

#endif /* ASM_TLB_H */
