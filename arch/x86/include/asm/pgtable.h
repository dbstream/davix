/**
 * Functions for page table manipulation.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <asm/page_defs.h>
#include <asm/pg_bits.h>

typedef uint64_t pteval_t;

enum page_cache_mode {
	pcm_writeback = 0,
	pcm_writethrough = 1,
	pcm_uc_minus = 2,
	pcm_uncached = 3,
	pcm_writecombine = 4
};

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

static inline pteval_t
make_io_pteval (page_cache_mode pcm)
{
	return pcm_pteval (pcm) | __PG_PRESENT | __PG_WRITE | x86_nx_bit;
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
} pte_t;

static inline pte_t
make_empty_pte (void)
{
	return { 0 };
}

static inline pte_t
__make_pte (uintptr_t phys_addr, pteval_t flags)
{
	return { phys_addr | flags };
}

static inline pte_t
make_pte (uintptr_t phys_addr, bool read, bool write, bool exec)
{
	return { phys_addr
		| ((read || write || exec) ? (__PG_PRESENT | __PG_USER) : 0)
		| (write ? __PG_WRITE : 0)
		| (exec || !(read || write) ? 0 : x86_nx_bit)
	};
}

static inline pte_t
make_pte_k (uintptr_t phys_addr, bool read, bool write, bool exec)
{
	return { phys_addr
		| ((read || write || exec) ? (__PG_PRESENT | __PG_GLOBAL) : 0)
		| (write ? __PG_WRITE : 0)
		| (exec || !(read || write) ? 0 : x86_nx_bit)
	};
}

static inline pte_t
make_io_pte (uintptr_t phys_addr, page_cache_mode pcm)
{
	return __make_pte (phys_addr, make_io_pteval (pcm));
}

template<int level>
constexpr static inline int
__pgtable_index (uintptr_t addr)
{
	return (addr >> (3 + 9 * level)) & 511;
}
