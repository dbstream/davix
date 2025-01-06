/**
 * Translation Lookaside Buffer tracking and flushing on x86.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/cpuset.h>
#include <davix/page.h>
#include <davix/smp.h>
#include <asm/cpulocal.h>
#include <asm/cregs.h>
#include <davix/mm.h>
#include <asm/mm.h>
#include <asm/mm_init.h>
#include <asm/tlb.h>
#include <davix/printk.h>

/**
 * For now, this is very simplistic code. We do not use PCIDs at all, and we
 * dispatch and wait on work in a dumb way.
 */

static void
tlb_flush_handler (void *arg)
{
	struct tlb *tlb = (struct tlb *) arg;
	unsigned long start = tlb->flush_range_start;
	unsigned long end = tlb->flush_range_end;

	if (start == end)
		/**
		 * If start == end, there are no addresses to flush.  However since the
		 * early return path of tlb_flush wasn't taken, there are some unmapped
		 * page tables we need to flush.  A single invlpg(0) will do this.
		 */
		__invlpg (NULL);
	else if (end - start < 48UL * PAGE_SIZE) {
		/**
		 * If the number of pages we are flushing is less than some
		 * amount, flush them in sequence with invlpg().
		 */
		for (; start < end; start += PAGE_SIZE)
			__invlpg ((void *) start);
	} else {
		/**
		 * We are flushing a large number of pages.
		 * Do this by writing to %cr4.
		 */
		bool flag = irq_save ();
		write_cr4 (__cr4_state ^ __CR4_PGE);
		write_cr4 (__cr4_state);
		irq_restore (flag);
	}
}

void
tlb_flush (struct tlb *tlb)
{
	/**
	 * It is concievable that tlb_flush is called without there being any
	 * pages that need to be flushed. In this case, do an early return.
	 */
	if (!tlb->flush_range_start && !tlb->flush_range_end
		&& list_empty (&tlb->pages_deferred))
			return;

	/** 
	 * Flush the TLB on all CPUs.
	 */
	for_each_online_cpu (cpu)
		smp_call_on_cpu (cpu, tlb_flush_handler, tlb);

	/**
	 * Free the TLB pages.
	 */
	while (!list_empty (&tlb->pages_deferred)) {
		struct pfn_entry *page = list_item (struct pfn_entry,
				pfn_free.list, tlb->pages_deferred.next);

		list_delete (&page->pfn_free.list);
		free_page (page, ALLOC_KERNEL);
	}

	/**
	 * Reinitialize the TLB tracker, in case the calling code expects to be
	 * able to continue using the same TLB tracker object after flushing.
	 */
	tlbflush_init (tlb, tlb->mm);
}

void
switch_to_mm (struct process_mm *mm)
{
	if (!mm) {
		write_cr3 (virt_to_phys (get_vmap_page_table ()));
		return;
	}

	unsigned long value = virt_to_phys (get_pgtable (mm->pgtable_handle));
	write_cr3 (value);
}
