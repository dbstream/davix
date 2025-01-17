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
 * This is the maximum page table level for which vunmap() should free page
 * tables.
 */
#define max_vunmap_pgtable_level (max_pgtable_level - 1)

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

/**
 * Allocate and initialize a new root page table for process_mm.
 */
extern void *
alloc_user_page_table (void);

static inline void
free_user_page_table (void *handle)
{
	free_page_table (max_pgtable_level, handle);
}

static inline pgtable_t *
get_pgtable (void *handle)
{
	return (pgtable_t *) handle;
}

/**
 * Get the root page table to be used for vmap operations.
 */
extern pgtable_t *
get_vmap_page_table (void);

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
 * Unconditionally install 'desired' into 'entry'.
 */
static inline void
__pte_install_always (pgtable_t *entry, pte_t desired)
{
	atomic_store_release (entry, desired);
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
 * Read a page table entry.
 */
static inline pte_t
__pte_read (pgtable_t *entry)
{
	return atomic_load_acquire (entry);
}

/**
 * Clear a page table entry. Returns the old value.
 */
static inline pte_t
__pte_clear (pgtable_t *entry)
{
	return atomic_exchange (entry, 0, memory_order_acquire);
}

/**
 * Get the address pointed to by a PTE.
 */
static inline unsigned long
pte_addr (int level, pte_t pte)
{
	if (level > 1)
		return pte & __PG_ADDR_MASK & ~__PG_PAT_HUGE;
	else
		return pte & __PG_ADDR_MASK;
}

static inline bool
pte_is_readable (int level, pte_t pte)
{
	(void) level;
	return (pte & __PG_PRESENT) ? true : false;
}

static inline bool
pte_is_writable (int level, pte_t pte)
{
	(void) level;
	return (pte & __PG_WRITE) ? true : false;
}

static inline bool
pte_update_needs_flush (pte_t old_pte, pte_t new_pte)
{
	/**
	 * For now, always flush if old_pte was present.  vm doesn't handle
	 * pagefaults yet
	 */

	(void) new_pte;
	return (old_pte & __PG_PRESENT) ? true : false;
}

/**
 * Convert from PROT_* bits to pte flags.
 */
static inline unsigned long
make_pte_flags (bool prot_read, bool prot_write, bool prot_exec)
{
	unsigned long flags = 0;
	if (prot_read || prot_write || prot_exec)
		flags |= __PG_PRESENT;
	if (prot_write)
		flags |= __PG_WRITE;
	if (!prot_exec)
		flags |= x86_nx_bit;
	return flags;
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

/**
 * Make a userspace PTE value from page table level, physical address,
 * and flags.
 */
static inline pte_t
make_user_pte (int level, unsigned long phys_addr, unsigned long flags)
{
	flags |= __PG_USER;
	if (level > 1)
		flags |= (flags & __PG_PAT) ? __PG_PAT_HUGE : __PG_HUGE;
	return phys_addr | flags;
}

static inline bool
pte_is_present (int level, pte_t pte)
{
	(void) level;
	return (pte & __PG_PRESENT) ? true : false;
}

static inline bool
pte_is_nonnull (int level, pte_t pte)
{
	(void) level;
	return pte ? true : false;
}

static inline bool
pte_is_pgtable (int level, pte_t pte)
{
	if (level == 1)
		return false;
	return (pte & __PG_HUGE) ? false : true;
}

static inline pgtable_t *
pte_pgtable (int level, pte_t pte)
{
	(void) level;
	return (pgtable_t *) phys_to_virt (pte & __PG_ADDR_MASK);
}

#endif /* _ASM_PGTABLE_H */
