/**
 * 'struct pfn_entry' - PFN database entry format
 * Copyright (C) 2024  dbstream
 */
#ifndef DAVIX_PAGE_H
#define DAVIX_PAGE_H 1

#include <davix/alloc_flags.h>
#include <davix/errno.h>
#include <davix/list.h>
#include <davix/spinlock.h>
#include <asm/page.h>

struct slab;

/**
 * This is the format of an entry in the PFN database. There is one of these for
 * every page in the system, so keep the structure small.
 */
struct pfn_entry {
	/**
	 * PFN flags. These must __always__ be accessed atomically. There are
	 * helper functions for this, set_page_flags and get_page_flags. Note
	 * that they do __not__ provide acquire-release semantics.
	 */
	unsigned long flags;
	union {
		struct {
			unsigned int nfree;
			void *pobj;
			struct slab *slab;
			struct list list;
		} pfn_slab;

		struct {
			struct list list;
		} pfn_free;

		unsigned long pad[(ARCH_PFN_ENTRY_SIZE / sizeof(long)) - 1];
	};
};

static inline void
set_page_flags (struct pfn_entry *entry, unsigned long flags)
{
	atomic_store_relaxed (&entry->flags, flags);
}

static inline unsigned long
get_page_flags (struct pfn_entry *entry)
{
	return atomic_load_relaxed (&entry->flags);
}

/* pfn_entry->flags */
/**
 * Bits in pfn_entry->flags:
 *	PFN_SLAB	Indicates that this page holds objects for the slab
 *			allocator.
 *	PFN_IO		IO is being performed on the page.
 *	PFN_IOPOISON	The page has been "poisoned" by bad IO (e.g. read from
 *			disk failing).
 *	PFN_UNINIT	The page is "uninitialized" and cannot be read from.
 */
#define PFN_SLAB	(1UL << 1)
#define PFN_IO		(1UL << 2)
#define PFN_IOPOISON	(1UL << 3)
#define PFN_UNINIT	(1UL << 4)

_Static_assert (sizeof (struct pfn_entry) == ARCH_PFN_ENTRY_SIZE, "pfn_entry");

/* Architectures provide us with pfn_to_pfn_entry and pfn_entry_to_pfn. */
#define phys_to_pfn_entry(x) pfn_to_pfn_entry(((unsigned long) x) / PAGE_SIZE)
#define pfn_entry_to_phys(x) (PAGE_SIZE * pfn_entry_to_pfn(x))
#define virt_to_pfn_entry(x) phys_to_pfn_entry(virt_to_phys(x))
#define pfn_entry_to_virt(x) phys_to_virt(pfn_entry_to_phys(x))

/**
 * Allocate one page of physical memory, where @flags is the set of
 * allocation flags to use.
 */
extern struct pfn_entry *
alloc_page (alloc_flags_t flags);

/**
 * Free one page of physical memory. If @flags is the same set of flags
 * that were used for allocation, accounting etc. will be fine.
 */
extern void
free_page (struct pfn_entry *page, alloc_flags_t flags);

/**
 * Reserve @num pages, if possible.
 * Return value:
 *   ESUCCESS	@num pages were successfully reserved and accounted.
 *   ENOMEM	there is not enough memory available (or too much is already reserved)
 */
extern errno_t
reserve_pages (unsigned long num);

/**
 * Unreserve @num pages.
 */
extern void
unreserve_pages (unsigned long num);

/**
 * Dump the current state of the page allocator to printk.
 */
extern void
pgalloc_dump (void);

#endif /* DAVIX_PAGE_H */
