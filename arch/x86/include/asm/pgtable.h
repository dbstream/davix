/**
 * Functions for page table manipulation.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <asm/page_defs.h>
#include <asm/pg_bits.h>

typedef uint64_t pteval_t;

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

template<int level>
constexpr static inline int
__pgtable_index (uintptr_t addr)
{
	return (addr >> (3 + 9 * level)) & 511;
}
