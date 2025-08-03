// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/tlb.cc
 * Translation cache invalidation routines.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/tlb.h>
#include <Ke/context.h>
#include <Mm/page_alloc.h>
#include <asm/cpufeature.h>
#include <asm/creg_access.h>
#include <asm/creg_bits.h>
#include <asm/invlpg.h>

/**
 * flush_tlb_full - flush all TLB entries and all paging-structure cache entries
 * on the current CPU.
 */
static void flush_tlb_full(void)
{
	KeDisableIRQs();
	/*
	 * Perform a full TLB and paging-structure cache flush by updating the
	 * CR4 register.
	 */
	write_cr4(__cr4_state ^ __CR4_PGE);
	write_cr4(__cr4_state);
	KeEnableIRQs();
}

/**
 * do_flush_tlb - flush the TLB on the current CPU.
 * @tlb: TLB_DATA structure with information about the addresses to flush
 */
static void do_flush_tlb(const TLB_DATA *tlb)
{
	if (!tlb->mm && !tlb->page_table_pages.empty()) {
		/*
		 * Because of recursive page tables which cause TLB entries
		 * without the global bit set to be created for kernel page
		 * table mappings, any removal of a kernel page table page
		 * requires a full TLB flush.
		 */
		flush_tlb_full();
	} else if (tlb->num_pages_to_flush > MAX_TLB_FLUSH_PAGES) {
		/*
		 * If the number of pages to flush is larger than the limit for
		 * __invlpg on the pages_to_flush array, do a full TLB flush.
		 */
		flush_tlb_full();
	} else if (tlb->num_pages_to_flush > 0) {
		/*
		 * Flush pages via INVLPG.
		 */
		for (unsigned long i = 0; i < tlb->num_pages_to_flush; i++) {
			__invlpg(tlb->pages_to_flush[i]);
		}
	} else {
		/*
		 * There are no pages to flush but paging-structure caches still
		 * need flushing.  Perform a dummy INVLPG on address zero to
		 * achieve this effect.
		 */
		__invlpg(0);
	}
}

static void flush_tlb_on_all_cpus(TLB_DATA *tlb)
{
	// FIXME: make this SMP
	do_flush_tlb(tlb);
}

void HalEndTLB(TLB_DATA *tlb)
{
	if (tlb->num_pages_to_flush == 0 && tlb->page_table_pages.empty())
		return;

	flush_tlb_on_all_cpus(tlb);

	MMPFN *pfn;
	while ((pfn = tlb->page_table_pages.pop_front()))
		MmFreePage(pfn);
}

