/**
 * kmap_fixed:  allocation-free memory mappings that can be used during boot.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <asm/pg_bits.h>
#include <asm/pgtable.h>

extern "C" { extern volatile uint64_t __kmap_fixed_page[]; }

enum { KMAP_FIXED_BASE = UINT64_C(0xffffffffffe00000) };

enum {
	KMAP_FIXED_IDX_LOCAL_APIC = 0,
	KMAP_FIXED_IDX_P1D,
	KMAP_FIXED_IDX_P2D,
	KMAP_FIXED_IDX_P3D,
	KMAP_FIXED_IDX_P4D,
	KMAP_FIXED_IDX_P5D,
	KMAP_FIXED_IDX_SETUP_TMP,
	KMAP_FIXED_IDX_FIRST_DYNAMIC
};

constexpr static inline uintptr_t
kmap_fixed_address (int idx)
{
	return KMAP_FIXED_BASE + idx * PAGE_SIZE;
}

void
kmap_fixed_clear (int idx);

void *
kmap_fixed_install (int idx, pte_t value);

int
kmap_fixed_alloc_indices (int num);

void
kmap_fixed_free_indices (int idx);

void *
kmap_fixed (uintptr_t phys, size_t size, pteval_t flags);

void
kunmap_fixed (void *ptr);
