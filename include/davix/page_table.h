/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_PAGE_TABLE_H
#define __DAVIX_PAGE_TABLE_H

#include <davix/mm_types.h>

/*
 * Page table API.
 *
 * Page tables can be modified by calling the pgop* and *_pte functions.
 *
 * Generally speaking, these functions don't validate their inputs, i.e. don't
 * pass in random junk that you haven't checked for correctness to these
 * functions.
 */

/*
 * struct pgop - Managing the TLB when modifying page tables.
 */
struct pgop {
	/*
	 * A memory range that must be flushed when we are done with our page
	 * table modifications.
	 *
	 * pgop_begin() will initialize this to something appropriate.
	 */
	unsigned long flush_start;
	unsigned long flush_end;

	/*
	 * A linked list of pages that we will free, but that are possibly still
	 * in the TLB and thus cannot be freed immedietally.
	 *
	 * pgop_begin() will initialize this to something appropriate.
	 */
	struct list delayed_pages;

	/*
	 * The mm object we are modifying.
	 *
	 * If this is ``&kernelmode_mm``, we are mapping kernel pages and should
	 * make them inaccessible to user-mode.
	 */
	struct mm *mm;

	/*
	 * Hugepage shift.
	 */
	unsigned shift;
};

/*
 * Test if huge pages of size ``PAGE_SIZE << shift`` are supported.
 */
int has_hugepage_size(unsigned shift);

/*
 * Begin/end a page table operation. Calls to these functions must come in pairs.
 *
 * ``shift`` specifies the hugepage size.
 */
void pgop_begin(struct pgop *pgop, struct mm *mm, unsigned shift);
void pgop_end(struct pgop *pgop);

/*
 * Allocate PTEs from ``start_addr`` to ``end_addr``.
 *
 * Returns non-zero on failure. This function has no side effects on the page
 * tables other than to maybe put some pages into the delayed free queue.
 */
int alloc_ptes(struct pgop *pgop,
	unsigned long start_addr, unsigned long end_addr);

/*
 * Free PTEs from ``start_addr`` to ``end_addr``.
 *
 * NOTE: Remember to call clear_pte() for all pages within the range.
 */
void free_ptes(struct pgop *pgop,
	unsigned long start_addr, unsigned long end_addr);

/*
 * Set the value of a page table entry to a physical address, cache mode and
 * page protection mode.
 *
 * A pgmode_t of 0 indicates non-present.
 */
void set_pte(struct pgop *pgop, unsigned long addr, unsigned long phys_addr,
	pgcachemode_t cachemode, pgmode_t mode);

static inline void clear_pte(struct pgop *pgop, unsigned long addr)
{
	set_pte(pgop, addr, 0, PG_WRITEBACK, 0);
}

#endif /* __DAVIX_PAGE_TABLE_H */
