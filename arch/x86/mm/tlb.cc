/**
 * Translation Lookaside Buffer (TLB) management.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/asm.h>
#include <asm/creg_bits.h>
#include <asm/irql.h>
#include <asm/pgtable_modify.h>
#include <davix/cpuset.h>
#include <davix/page.h>
#include <davix/smp.h>

pte_t *
alloc_pgtable (int level)
{
	(void) level;
	Page *page = alloc_page (ALLOC_KERNEL | __ALLOC_ZERO);
	return page ? (pte_t *) page_to_virt (page) : nullptr;
}

void
free_pgtable (pte_t *table, int level)
{
	(void) level;
	free_page (virt_to_page ((uintptr_t) table));
}

static inline bool
tlb_accumulator_empty (tlb_accumulator *tlb)
{
	return tlb->flush_range_start == tlb->flush_range_end
			&& tlb->deferred_pages.empty ();
}

static void
do_flush_tlb (tlb_accumulator *tlb)
{
	uintptr_t start = tlb->flush_range_start;
	uintptr_t end = tlb->flush_range_end;

	if (start == end)
		__invlpg (0);
	else if (end - start < 64 * PAGE_SIZE) {
		for (; start < end; start += PAGE_SIZE)
			__invlpg (start);
	} else {
		disable_irq ();
		write_cr4 (__cr4_state ^ __CR4_PGE);
		write_cr4 (__cr4_state);
		enable_irq ();
	}
}

static void
flush_tlb_one (void *tlb)
{
	do_flush_tlb ((tlb_accumulator *) tlb);
}

void
tlb_end_kernel (tlb_accumulator *tlb)
{
	if (tlb_accumulator_empty (tlb))
		return;

	/*
	 * TODO: this is ridiculously inefficient and doesn't scale at all.  Do
	 * something better in the future, e.g. via some smp_call_on_every_cpu.
	 */
	for (unsigned int cpu : cpu_online)
		smp_call_on_cpu (cpu, flush_tlb_one, tlb);

	while (!tlb->deferred_pages.empty ())
		free_page (tlb->deferred_pages.pop_front ());

	/*
	 * Call tlb_begin_kernel in case someone accidentally continues using
	 * this TLB accumulator without calling tlb_begin_kernel.
	 */
	tlb_begin_kernel (tlb);
}
