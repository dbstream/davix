// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/tlb.h
 * Translation cache invalidation operations.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Mm/pfn.h>

struct MMSTRUCT;

/*
 * MAX_TLB_FLUSH_PAGES: the number of TLB entries that can be flushed without
 * falling back to full TLB invalidation.
 */
static constexpr unsigned long MAX_TLB_FLUSH_PAGES = 32;

struct TLB_DATA {
	MMSTRUCT *mm;

	unsigned long pages_to_flush[MAX_TLB_FLUSH_PAGES];
	unsigned long num_pages_to_flush;

	MMPFN_List page_table_pages;
};

void HalEndTLB(TLB_DATA *tlb);

static inline void HalBeginTLB(TLB_DATA *tlb, MMSTRUCT *mm)
{
	tlb->mm = mm;
	tlb->num_pages_to_flush = 0;
	tlb->page_table_pages.init();
}

/**
 * HalFlushTLB - flush a TLB entry.
 * @tlb: TLB_DATA structure
 * @address: virtual address of TLB entry to flush
 */
static inline void HalFlushTLB(TLB_DATA *tlb, unsigned long address)
{
	if (tlb->num_pages_to_flush < MAX_TLB_FLUSH_PAGES)
		tlb->pages_to_flush[tlb->num_pages_to_flush] = address;
	tlb->num_pages_to_flush++;
}

/**
 * HalFlushAndFreeTableTLB - flush a page table from the TLB.
 * @tlb: TLB_DATA structure
 * @level: page table level
 * @address: virtual address managed by this page table
 * @page: MMPFN of page table page
 */
static inline void HalFlushAndFreeTableTLB(TLB_DATA *tlb,
		int level, unsigned long address, MMPFN *page)
{
	(void) level;
	(void) address;

	tlb->page_table_pages.push_back(page);
}

