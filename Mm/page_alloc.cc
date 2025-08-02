// SPDX-License-Identifier: GPL-3.0
/*
 * File: Mm/page_alloc.cc
 * Page allocator.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ke/spinlock.h>
#include <Mm/page_alloc.h>
#include <Mm/pfn.h>
#include <dsl/list.h>

/**
 * pgalloc_lock: the global page allocator spinlock.
 */
static KeSpinlock pgalloc_lock;

static MMPFN_List page_free_list;

/**
 * MmFreePage - put a page on the global page free list.
 * @pfn: page to 'free' to the free list.
 */
void MmFreePage(MMPFN *pfn)
{
	pgalloc_lock.lock();
	page_free_list.push_front(pfn);
	pgalloc_lock.unlock();
}

/**
 * MmAllocatePage - allocate a page from the global page free list.
 */
MMPFN *MmAllocatePage(void)
{
	pgalloc_lock.lock();
	MMPFN *pfn = page_free_list.pop_front(); // returns nullptr if empty
	pgalloc_lock.unlock();
	return pfn;
}

