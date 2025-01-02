/**
 * Translation Lookaside Buffer tracking and flushing.
 * Copyright (C) 2024-present  dbstream
 */
#ifndef _ASM_TLB_H
#define _ASM_TLB_H 1

#include <davix/list.h>
#include <davix/stddef.h>
#include <davix/page.h>
#include <asm/pgtable.h>

struct pfn_entry;
struct process_mm;

static inline void
__invlpg (void *mem)
{
	asm volatile ("invlpg (%0)" :: "r" (mem) : "memory");
}

struct tlb {
	unsigned long flush_range_start;
	unsigned long flush_range_end;

	struct list pages_deferred;

	struct process_mm *mm;
};

/**
 * Initialize a TLB tracker object.
 *
 * @tlb		TLB tracker storage
 * @mm		process address space (NULL for kernel)
 */
static inline void
tlbflush_init (struct tlb *tlb, struct process_mm *mm)
{
	tlb->flush_range_start = 0;
	tlb->flush_range_end = 0;
	list_init (&tlb->pages_deferred);
	tlb->mm = mm;
}

/**
 * Track the freeing of a page. The page will be freed when the TLB is flushed.
 *
 * @tlb		TLB tracker storage
 * @page	page to free
 */
static inline void
tlb_free_page (struct tlb *tlb, struct pfn_entry *page)
{
	list_insert (&tlb->pages_deferred, &page->pfn_free.list);
}

/**
 * Flush a TLB range.
 *
 * @tlb		TLB tracker storage
 * @start	Start address of the range
 * @end		End address of the range, non-inclusive
 */
static inline void
tlb_flush_range (struct tlb *tlb, unsigned long start, unsigned long end)
{
	if (!tlb->flush_range_start && !tlb->flush_range_end) {
		tlb->flush_range_start = start;
		tlb->flush_range_end = end;
	} else {
		tlb->flush_range_start = min (unsigned long, tlb->flush_range_start, start);
		tlb->flush_range_end = max (unsigned long, tlb->flush_range_end, end);
	}
}

/**
 * Flush a page table.
 *
 * @tlb		TLB tracker storage
 * @table	Page table object
 * @level	Level in the paging hierarchy of the page table
 * @addr	Virtual address of the memory managed by the page table object
 */
static inline void
tlb_flush_pgtable (struct tlb *tlb, pgtable_t *table, int level, unsigned long addr)
{
	(void) level;
	(void) addr;

	tlb_free_page (tlb, virt_to_pfn_entry (table));
}

/**
 * Perform a TLB flush. This finalizes the TLB tracker object and frees
 * associated resources.
 */
extern void
tlb_flush (struct tlb *tlb);

#endif /* _ASM_TLB_H */
