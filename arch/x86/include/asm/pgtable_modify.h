/**
 * Page table modification.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <asm/cpufeature.h>
#include <asm/pgtable.h>
#include <davix/atomic.h>
#include <davix/page.h>

struct tlb_accumulator {
	uintptr_t flush_range_start;
	uintptr_t flush_range_end;
	PageList deferred_pages;
};

static inline void
tlb_begin_kernel (tlb_accumulator *tlb)
{
	tlb->flush_range_start = 0;
	tlb->flush_range_end = 0;
}

void
tlb_end_kernel (tlb_accumulator *tlb);

static inline void
tlb_add_range (tlb_accumulator *tlb, uintptr_t start, uintptr_t end)
{
	if (tlb->flush_range_start == tlb->flush_range_end) {
		tlb->flush_range_start = start;
		tlb->flush_range_end = end;
	} else {
		if (start < tlb->flush_range_start)
			tlb->flush_range_start = start;
		if (end > tlb->flush_range_end)
			tlb->flush_range_end = end;
	}
}

static inline void
tlb_add_page (tlb_accumulator *tlb, Page *page)
{
	tlb->deferred_pages.push_back (page);
}

static inline void
tlb_add_pgtable (tlb_accumulator *tlb, pte_t *table, int level)
{
	(void) level;
	tlb_add_page (tlb, virt_to_page ((uintptr_t) table));
}

static inline void
pte_clear (pte_t *pte)
{
	atomic_store_relaxed (&pte->value, make_empty_pte().value);
}

static inline void
pte_install (pte_t *pte, pte_t entry)
{
	atomic_store_release (&pte->value, entry.value);
}

static inline pte_t
pte_read (pte_t *pte)
{
	pteval_t value = atomic_load_relaxed (&pte->value);
	return { value };
}

static inline bool
pgtable_install (pte_t *pte, pte_t &value)
{
	pte_t expected = make_empty_pte ();
	bool success = atomic_cmpxchg (&pte->value,
			&expected.value, value.value,
			mo_acq_rel, mo_acquire);
	if (!success)
		value = expected;
	return success;
}

static inline int
max_pgtable_level (void)
{
	return has_feature (FEATURE_LA57) ? 5 : 4;
}

pte_t *
alloc_pgtable (int level);

void
free_pgtable (pte_t *table, int level);

pte_t *
get_vmap_pgtable (void);

static inline pte_t *
get_vmap_pgtable_entry (uintptr_t addr)
{
	return get_vmap_pgtable () + __pgtable_index (addr, max_pgtable_level ());
}

static inline pte_t *
pte_pgtable (pte_t value)
{
	return (pte_t *) phys_to_virt (value.phys_addr ());
}

static inline pte_t *
pgtable_entry (pte_t value, uintptr_t addr, int level)
{
	return pte_pgtable (value) + __pgtable_index (addr, level);
}

static inline uintptr_t
pgtable_boundary_next (uintptr_t start, uintptr_t end, int level)
{
	uintptr_t entry_size = __pgtable_entry_size (level);
	start &= ~(entry_size - 1);
	start += entry_size;
	if (end && end < start)
		start = end;
	return start;
}
