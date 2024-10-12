/**
 * Functions for accessing and modifying page tables on x86.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_PGTABLE_H
#define _ASM_PGTABLE_H 1

#include <davix/atomic.h>
#include <davix/stdbool.h>
#include <davix/stddef.h>
#include <asm/page_defs.h>

/**
 * pgtable_t is the type of an entry within the page tables. As such, functions
 * that operate on page tables or page table entries most likely take pointers
 * to pgtable_t as arguments.
 */
typedef unsigned long pgtable_t;

/**
 * pte_t represents the value of a page table entry.
 */
typedef unsigned long pte_t;

/**
 * Allocate a new page table. 'level' indicates the level in the page-table
 * hierarchy that we are allocating.
 */
extern pgtable_t *
alloc_page_table (int level);

/**
 * Free the page table pointed to by 'table'.
 *
 * WARNING: Don't use this when removing page tables. Instead, let it be handled
 * properly by the TLB code.
 */
extern void
free_page_table (int level, pgtable_t *table);

	/* Functions for manipulating page tables. */

/**
 * Install '*new_table' into 'entry'. Returns '*new_table' if it was installed,
 * otherwise returns an already existing page table.
 *
 * 'kernel' should be set to true if kernel page tables are being manipulated,
 * false otherwise.
 *
 * If the new page table is installed, '*new_table' will be set to NULL.
 */
static inline pgtable_t *
__pgtable_install (pgtable_t *entry, pgtable_t **new_table, bool kernel)
{
	unsigned long expected = 0, desired = virt_to_phys (*new_table);
	desired |= kernel ? PAGE_KERNEL_PGTABLE : PAGE_USER_PGTABLE;

	if (atomic_cmpxchg_acqrel_acq (entry, &expected, desired)) {
		expected = desired;
		*new_table = NULL;
	}

	return (pgtable_t *) phys_to_virt(expected & __PG_ADDR_MASK);
}

/**
 * Install 'desired' into 'entry', which currently holds the value 'expected'.
 * Returns the pre-existing value of *entry. If it doesn't match with what was
 * expected, 'desired' was not installed.
 */
static inline pte_t
__pte_install (pgtable_t *entry, pte_t expected, pte_t desired)
{
	atomic_cmpxchg_acqrel_acq (entry, &expected, desired);
	return expected;
}

/**
 * Make a PTE value from page table level, physical address, and flags.
 */
static inline pte_t
make_pte (int level, unsigned long phys_addr, unsigned long flags)
{
	if (level > 1)
		flags |= (flags & __PG_PAT) ? __PG_PAT_HUGE : __PG_HUGE;
	return phys_addr | flags;
}

#endif /* _ASM_PGTABLE_H */
