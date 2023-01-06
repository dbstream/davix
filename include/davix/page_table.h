/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_PAGE_TABLE_H
#define __DAVIX_PAGE_TABLE_H

#include <davix/types.h>

typedef enum pgcachemode {
	PG_WRITEBACK,		/* Normal, "cacheable" memory. Use this when possible. */
#ifdef CONFIG_HAVE_PGCACHEMODE_WRITETHROUGH
	PG_WRITETHROUGH,	/* Cache reads but not writes. */
#endif
#ifdef CONFIG_HAVE_PGCACHEMODE_WRITECOMBINING
	PG_WRITECOMBINE,	/* Don't cache anything, but do combine consecutive writes. */
#endif
#ifdef CONFIG_HAVE_PGCACHEMODE_UNCACHED_MINUS
	PG_UNCACHED_MINUS,	/* Same as uncached, but can be overridden by platform. */
#endif
	PG_UNCACHED,		/* Don't cache anything. */
	NUM_PG_CACHEMODES
} pgcachemode_t;

/*
 * Fall-back to PG_UNCACHED if arch doesn't provide one of the above.
 */
#ifndef CONFIG_HAVE_PGCACHEMODE_WRITETHROUGH
#define PG_WRITETHROUGH PG_UNCACHED
#endif

#ifndef CONFIG_HAVE_PGCACHEMODE_WRITECOMBINING
#define PG_WRITECOMBINING PG_UNCACHED
#endif

#ifndef CONFIG_HAVE_PGCACHEMODE_UNCACHED_MINUS
#define PG_UNCACHED_MINUS PG_UNCACHED
#endif

typedef bitwise unsigned long pgmode_t;

#define PG_READABLE	((force pgmode_t) (1 << 0))	/* PROT_READ */
#define PG_WRITABLE	((force pgmode_t) (1 << 1))	/* PROT_WRITE */
#define PG_EXECUTABLE	((force pgmode_t) (1 << 2))	/* PROT_EXEC */

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
	 * The mm object we are modifying. This must be set by the caller of
	 * pgop_begin(), as it is a parameter to it.
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
void pgop_begin(struct pgop *pgop, unsigned shift);
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
