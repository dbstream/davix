// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/page_tables.h
 * HAL functions for page table access and manipulation.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#define DEBUG_PAGETABLES 1

#ifndef DEBUG_PAGETABLES
#define DEBUG_PAGETABLES 0
#endif

#include <Hal/mm.h>
#include <Hal/pteflags.h>
#include <asm/cpufeature.h>
#include <asm/pg_bits.h>
#include <davix/atomic.h>
#include <davix/bug.h>

typedef struct _MMPTE_TAG { unsigned long value; } MMPTE, *MMPTEP;

static constexpr int HAL_PTES_PER_PAGE = 512;

extern unsigned long MiPTEAddressMask;

static inline MMPTEP MmGetPteForAddress(unsigned long address)
{
	return &MiPTEBase[(address & MiPTEAddressMask) >> PAGE_SHIFT];
}

static inline MMPTEP MmGetPteForPtr(void *ptr)
{
	return MmGetPteForAddress((unsigned long) ptr);
}

static inline int HalNumPageTableLevels(void)
{
	if (CPUFeature(LA57))
		return 5;
	else
		return 4;
}

static inline MMPTEP MmGetPteForAddressLevel(unsigned long address, int level)
{
#if DEBUG_PAGETABLES
	BUG_ON(level < 1);
	BUG_ON(level > HalNumPageTableLevels());
#endif

	address &= MiPTEAddressMask;
	address >>= (3 + 9 * level);

	return &MiPTEBaseForLevelIndex[level - 1][address];
}

static inline int HalMaxHugePTELevel(void)
{
	if (CPUFeature(PDPE1GB))
		return 3;
	else
		return 2;
}

constexpr static inline unsigned long HalPTESize(int level)
{
#if DEBUG_PAGETABLES
	BUG_ON(level < 1);
	BUG_ON(level > HalMaxHugePTELevel());
#endif

	return 1UL << (3 + 9 * level);
}

/**
 * HalReadPTE - read a page table entry (PTE).
 * @ptep: pointer to PTE
 */
static inline MMPTE HalReadPTE(MMPTEP ptep)
{
	return { atomic_load_acquire(&ptep->value) };
}

/**
 * HalWritePTE - write a page table entry (PTE).
 * @ptep: pointer to PTE
 * @value: new PTE value
 */
static inline void HalWritePTE(MMPTEP ptep, MMPTE value)
{
	atomic_store_release(&ptep->value, value.value);
}

/**
 * HalClearPTE - zero out a page table entry (PTE).
 * @ptep: pointer to PTE
 */
static inline void HalClearPTE(MMPTEP ptep)
{
	atomic_store_relaxed(&ptep->value, 0);
}

/**
 * HalMakeKPTE - make a kernel page table entry.
 * @level: one-indexed page table level
 * @address: physical address of page
 * @flags: page table flags
 */
static inline MMPTE HalMakeKPTE(int level, unsigned long address, PTEFLAGS flags)
{
#if DEBUG_PAGETABLES
	BUG_ON(level < 1);
	BUG_ON(level > HalMaxHugePTELevel());
#endif

	if (level > 1) {
		if (flags & __PG_PAT)
			flags |= __PG_PAT_HUGE;
		else
			flags |= __PG_HUGE;
	}

	return { address | flags | __PG_GLOBAL };
}

/**
 * HalMakePTE - make a userspace page table entry.
 * @level: one-indexed page table level
 * @address: physical address of page
 * @flags: page table flags
 */
static inline MMPTE HalMakePTE(int level, unsigned long address, PTEFLAGS flags)
{
#if DEBUG_PAGETABLES
	BUG_ON(level < 1);
	BUG_ON(level > HalMaxHugePTELevel());
#endif

	if (level > 1) {
		if (flags & __PG_PAT)
			flags |= __PG_PAT_HUGE;
		else
			flags |= __PG_HUGE;
	}

	return { address | flags | __PG_USER };
}

/**
 * HalMakeTableKPTE - make a kernel page table PTE.
 * @level: one-indexed page table level
 * @address: physical address of page table page
 */
static inline MMPTE HalMakeTableKPTE(int level, unsigned long address)
{
#if DEBUG_PAGETABLES
	BUG_ON(level < 2);
	BUG_ON(level > HalNumPageTableLevels());
#endif

	return { address | __KERNEL_PAGE_TABLE };
}

/**
 * HalMakeTablePTE - make a userspace page table PTE.
 * @level: one-indexed page table level
 * @address: physical address of page table page
 */
static inline MMPTE HalMakeTablePTE(int level, unsigned long address)
{
#if DEBUG_PAGETABLES
	BUG_ON(level < 2);
	BUG_ON(level > HalNumPageTableLevels());
#endif

	return { address | __USER_PAGE_TABLE };
}

/**
 * HalPTEUpdateNeedsFlush - check if a PTE update requires TLB flushing.
 * @oldpte: old PTE value
 * @newpte: new PTE value
 */
static inline bool HalPTEUpdateNeedsFlush(MMPTE oldpte, MMPTE newpte)
{
	oldpte.value &= ~(__PG_ACCESSED | __PG_DIRTY);
	return oldpte.value != newpte.value;
}

/**
 * HalPTEEmpty - check if a PTE is considered empty.
 * @pte: PTE value
 */
static inline bool HalPTEEmpty(MMPTE pte)
{
	/*
	 * Apparently there is an erratum on some CPUs where __PG_ACCESSED and
	 * __PG_DIRTY can be set on empty PTEs.  Ignore these bits.
	 */
	pte.value &= ~(__PG_ACCESSED | __PG_DIRTY);
	return pte.value == 0;
}

/**
 * HalPTEHuge - check if a PTE is a huge page.
 * @pte: PTE value
 * @level: the page table level of this PTE
 *
 * If level is 1, this function always returns true.
 */
static inline bool HalPTEHuge(MMPTE pte, int level)
{
	if (level == 1)
		return true;

#if DEBUG_PAGETABLES
	BUG_ON(level < 1);
	BUG_ON(level > HalNumPageTableLevels());
#endif

	return pte.value & __PG_HUGE;
}

/**
 * HalPTEAddress - get the address pointed to by a non-huge-page PTE.
 * @pte: PTE value
 * @level: the page table level of this PTE
 *
 * If @level is greater than 1, @pte is a table PTE and the physical address of
 * the pointed-to page table is returned.
 */
static inline unsigned long HalPTEAddress(MMPTE pte, int level)
{
#if DEBUG_PAGETABLES
	BUG_ON(level < 1);
	BUG_ON(level > HalNumPageTableLevels());
	BUG_ON(level > 1 && (pte.value & __PG_HUGE));
#endif

	return pte.value & __PG_ADDR_MASK;
}

/**
 * HalPTEHugeAddress - get the address pointed to by a huge PTE.
 * @pte: PTE value
 * @level: the page table level of this PTE
 */
static inline unsigned long HalPTEHugeAddress(MMPTE pte, int level)
{
#if DEBUG_PAGETABLES
	BUG_ON(level < 1);
	BUG_ON(level > HalMaxHugePTELevel());
	BUG_ON(level > 1 && !(pte.value & __PG_HUGE));
#endif

	unsigned long entry_size = HalPTESize(level);
	return pte.value & __PG_ADDR_MASK & ~(entry_size - 1);
}

