/**
 * 'struct pfn_entry' - PFN database entry format
 * Copyright (C) 2024  dbstream
 */
#ifndef DAVIX_PAGE_H
#define DAVIX_PAGE_H 1

#include <davix/alloc_flags.h>
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
			spinlock_t lock;
			unsigned int nfree;
			void *pobj;
			struct slab *slab;
			struct list list;
		} pfn_slab;

		struct {
			struct list list;
			unsigned int page_order;
		} pfn_buddy;

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
#define PFN_BUDDY	(1UL << 0)
#define PFN_SLAB	(1UL << 1)

_Static_assert (sizeof (struct pfn_entry) == ARCH_PFN_ENTRY_SIZE, "pfn_entry");

/* Architectures provide us with pfn_to_pfn_entry and pfn_entry_to_pfn. */
#define phys_to_pfn_entry(x) pfn_to_pfn_entry(((unsigned long) x) / PAGE_SIZE)
#define pfn_entry_to_phys(x) (PAGE_SIZE * pfn_entry_to_pfn(x))
#define virt_to_pfn_entry(x) phys_to_pfn_entry(virt_to_phys(x))
#define pfn_entry_to_virt(x) phys_to_virt(pfn_entry_to_phys(x))

extern struct pfn_entry *
alloc_page (alloc_flags_t flags, unsigned int order);

extern void
free_page (struct pfn_entry *, unsigned int order);

#endif /* DAVIX_PAGE_H */
