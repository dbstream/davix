/**
 * Page-related definitions.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>

#define KERNEL_START 0xffffffff80000000UL
#define PAGE_SIZE UINT64_C(0x1000)
#define P1D_SIZE UINT64_C(0x200000)
#define P2D_SIZE UINT64_C(0x40000000)

struct Page;

extern uint64_t x86_nx_bit;
extern uint64_t PG_WT;
extern uint64_t PG_UC_MINUS;
extern uint64_t PG_UC;
extern uint64_t PG_WC;

extern uintptr_t HHDM_OFFSET;
extern Page *page_map;
extern uintptr_t USER_VM_FIRST, USER_VM_LAST;
extern uintptr_t KERNEL_VM_FIRST, KERNEL_VM_LAST;

typedef uintptr_t pfn_t;

static inline uintptr_t
phys_to_virt (uintptr_t x)
{
	return x + HHDM_OFFSET;
}

static inline uintptr_t
virt_to_phys (uintptr_t x)
{
	return x - HHDM_OFFSET;
}
