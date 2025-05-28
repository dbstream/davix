/**
 * Translation Lookaside Buffer (TLB) management.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/asm.h>
#include <asm/creg_bits.h>
#include <asm/irql.h>
#include <asm/pgtable_modify.h>
#include <davix/page.h>

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

void
tlb_end_kernel (tlb_accumulator *tlb)
{
	if (tlb_accumulator_empty (tlb))
		return;

	// also flush the TLB on other CPUs
	do_flush_tlb (tlb);

	while (!tlb->deferred_pages.empty ())
		free_page (tlb->deferred_pages.pop_front ());

	// in case someone forgets it...
	tlb_begin_kernel (tlb);
}
