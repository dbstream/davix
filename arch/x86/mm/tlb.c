/* SPDX-License-Identifier: MIT */
#include <davix/mm_types.h>
#include <davix/page_alloc.h>
#include <davix/page_table.h>
#include <davix/list.h>
#include <asm/page.h>
#include <asm/tlb.h>

/*
 * x86 Translation Lookaside Buffer management.
 */

/*
 * tlb_flush_local: flush the TLB on this CPU.
 */
static void tlb_flush_local(struct pgop *pgop)
{
	if(pgop->flush_start == pgop->flush_end) {
		if(!list_empty(&pgop->delayed_pages))
			invlpg(0);
		return;
	}

	for(unsigned long start = pgop->flush_start; start < pgop->flush_end;
		start += (PAGE_SIZE << pgop->shift))
			invlpg(start);
}

void tlb_flush(struct pgop *pgop)
{
	tlb_flush_local(pgop);

	struct page *page, *tmp;
	list_for_each_safe(page, &pgop->delayed_pages,
		pte.delayed_free_list, tmp) {
			list_del(&page->pte.delayed_free_list);
			free_page(page, 0);
	}

	pgop->flush_start = 0;
	pgop->flush_end = 0;
}
