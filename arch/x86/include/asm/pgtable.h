/**
 * Functions for page table manipulation.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <asm/page_defs.h>
#include <asm/pg_bits.h>

#define PAGE_KERNEL_PGTABLE	(__PG_PRESENT | __PG_WRITE)
#define PAGE_USER_PGTABLE	(__PG_PRESENT | __PG_WRITE | __PG_USER)

#define PAGE_KERNEL_TEXT	(__PG_PRESENT | __PG_GLOBAL)
#define PAGE_KERNEL_RODATA	(__PG_PRESENT | __PG_GLOBAL | x86_nx_bit)
#define PAGE_KERNEL_DATA	(__PG_PRESENT | __PG_WRITE | __PG_GLOBAL | x86_nx_bit)

typedef uint64_t pteval_t;

enum page_cache_mode {
	pcm_writeback = 0,
	pcm_writethrough = 1,
	pcm_uc_minus = 2,
	pcm_uncached = 3,
	pcm_writecombine = 4
};

static constexpr page_cache_mode PCM_NORMAL_RAM = pcm_writeback;

static inline pteval_t
pcm_pteval (page_cache_mode pcm)
{
	switch (pcm) {
	case pcm_writethrough:	return PG_WT;
	case pcm_uc_minus:	return PG_UC_MINUS;
	case pcm_uncached:	return PG_UC;
	case pcm_writecombine:	return PG_WC;
	default:		return 0;
	}
}

typedef struct pte_t {
	pteval_t value;

	constexpr inline uintptr_t
	phys_addr (void) const
	{
		return value & __PG_ADDR_MASK;
	}

	constexpr inline bool
	readable (void) const
	{
		return (value & __PG_PRESENT) ? true : false;
	}

	constexpr inline bool
	writable (void) const
	{
		return (value & (__PG_PRESENT | __PG_WRITE)) == (__PG_PRESENT | __PG_WRITE);
	}

	constexpr inline bool
	empty (void) const
	{
		return !value;
	}
} pte_t;

static inline pte_t
make_empty_pte (void)
{
	return { 0 };
}

static inline pte_t
make_pte_pgtable (pte_t *table)
{
	return { virt_to_phys ((uintptr_t) table) | PAGE_USER_PGTABLE };
}

static inline pte_t
make_pte_pgtable_k (pte_t *table)
{
	return { virt_to_phys ((uintptr_t) table) | PAGE_KERNEL_PGTABLE };
}

static inline pte_t
make_pte (uintptr_t phys_addr, pteval_t flags)
{
	if (flags & __PG_PRESENT)
		flags |= __PG_USER;

	return { phys_addr | flags };
}

static inline pte_t
make_pte_k (uintptr_t phys_addr, pteval_t flags)
{
	if (flags & __PG_PRESENT)
		flags |= __PG_GLOBAL;

	return { phys_addr | flags };
}

static inline pteval_t
make_io_pteval (page_cache_mode pcm)
{
	return PAGE_KERNEL_DATA | pcm_pteval (pcm);
}

static inline pte_t
make_io_pte (uintptr_t phys_addr, page_cache_mode pcm)
{
	return make_pte_k (phys_addr, make_io_pteval (pcm));
}

constexpr static inline int
__pgtable_index (uintptr_t addr, int level)
{
	return (addr >> (3 + 9 * level)) & 511;
}

constexpr static inline uintptr_t
__pgtable_entry_size (int level)
{
	return 1UL << (3 + 9 * level);
}
