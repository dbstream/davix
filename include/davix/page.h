/**
 * struct Page
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <asm/page_defs.h>
#include <davix/allocation_class.h>
#include <dsl/list.h>

static inline pfn_t
phys_to_pfn (uintptr_t phys)
{
	return phys / PAGE_SIZE;
}

static inline uintptr_t
pfn_to_phys (pfn_t pfn)
{
	return pfn * PAGE_SIZE;
}

#define virt_to_pfn(x) phys_to_pfn (virt_to_phys (x))
#define pfn_to_virt(x) phys_to_virt (pfn_to_phys (x))

struct SlabAllocator;

typedef unsigned long PageFlags;
enum : PageFlags {
	PAGE_SLAB			= 1UL << 0,
};

struct Page {
	dsl::ListHead node;
	unsigned long flags;
	union {
		struct {
			unsigned int nfree;
			void *pobj;
			SlabAllocator *allocator;
		} slab;
		long filler[5];
	};
};

static_assert (sizeof (Page) == 8 * sizeof (long));

static inline Page *
pfn_to_page (pfn_t pfn)
{
	return page_map + pfn;
}

static inline pfn_t
page_to_pfn (Page *page)
{
	return page - page_map;
}

#define phys_to_page(x) pfn_to_page (phys_to_pfn (x))
#define page_to_phys(x) pfn_to_phys (page_to_pfn (x))
#define virt_to_page(x) pfn_to_page (virt_to_pfn (x))
#define page_to_virt(x) pfn_to_virt (page_to_pfn (x))

typedef dsl::TypedList<Page, &Page::node> PageList;

void
pgalloc_init (void);

Page *
alloc_page (allocation_class aclass);

void
free_page (Page *page);

void
dump_pgalloc_stats (void);
