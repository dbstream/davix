/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_PAGE_ALLOC_H
#define __DAVIX_PAGE_ALLOC_H

#include <davix/types.h>
#include <davix/spinlock.h>

struct page;

#define NUM_ORDERS 10	/* 4KiB - 2MiB range of sizes */

struct page_freelist {
	struct list pages;
	unsigned long count;
};

struct page_alloc_zone {
	struct page_freelist freelists[NUM_ORDERS];
	struct page_alloc_zone *const fallback;
	spinlock_t lock;
	const char *const zone_name;
};

/*
 * What zone is appropriate for a particular set of alloc_flags?
 * Provided by arch.
 */
struct page_alloc_zone *alloc_flags_to_zone(alloc_flags_t flags);

/*
 * What zone does a page belong to?
 * Provided by arch.
 */
struct page_alloc_zone *page_to_zone(struct page *page);

/*
 * Iterate over page zones.
 * Provided by arch.
 */
struct page_alloc_zone *zoneiter_start(void);
struct page_alloc_zone *zoneiter_next(struct page_alloc_zone *zone);

/*
 * Initialize the page zones.
 * Provided by arch.
 */
void init_pagezones(void);

#define for_each_zone(zone) \
	for(struct page_alloc_zone *zone = zoneiter_start(); \
		zone; zone = zoneiter_next(zone))

struct page *alloc_page(alloc_flags_t flags, unsigned order);
void free_page(struct page *page, unsigned order);

void dump_pgalloc_info(void);

#endif /* __DAVIX_PAGE_ALLOC_H */
