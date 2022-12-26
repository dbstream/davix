/* SPDX-License-Identifier: MIT */
#include <davix/page_alloc.h>
#include <davix/list.h>
#include <asm/page.h>

static struct page_alloc_zone zone_low16M = {
	.fallback = NULL,
	.zone_name = "low 16MiB"
};

static struct page_alloc_zone zone_low4G = {
	.fallback = &zone_low16M,
	.zone_name = "low 4GiB"
};

static struct page_alloc_zone zone_normal = {
	.fallback = &zone_low4G,
	.zone_name = "normal"
};

struct page_alloc_zone *zoneiter_start(void)
{
	return &zone_normal;
}

struct page_alloc_zone *zoneiter_next(struct page_alloc_zone *zone)
{
	return zone->fallback;
}

void init_pagezones(void)
{
	for_each_zone(zone) {
		spinlock_init(&zone->lock);
		for(unsigned i = 0; i < NUM_ORDERS; i++) {
			zone->freelists[i] = (struct page_freelist) {
				.count = 0,
				.pages = LIST_INIT(zone->freelists[i].pages)
			};
		}
	}
}

struct page_alloc_zone *alloc_flags_to_zone(alloc_flags_t flags)
{
	if(flags & __ALLOC_LOW16M)
		return &zone_low16M;

	if(flags & __ALLOC_LOW4G)
		return &zone_low4G;

	return &zone_normal;
}

struct page_alloc_zone *page_to_zone(struct page *page)
{
	unsigned long addr = page_to_phys(page);
	if(addr < 16UL << 20)
		return &zone_low16M;

	if(addr < 4UL << 30)
		return &zone_low4G;

	return &zone_normal;
}
