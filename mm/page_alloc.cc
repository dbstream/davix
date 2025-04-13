/**
 * Page allocation.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/zone.h>
#include <davix/page.h>
#include <davix/printk.h>
#include <davix/spinlock.h>
#include <string.h>

struct Zone {
	PageList free_list;
	size_t count;
};

static Zone zone_list[num_page_zones];
static size_t total_free_pages;

static spinlock_t freelist_lock;

void
pgalloc_init (void)
{
	for (int i = 0; i < num_page_zones; i++) {
		zone_list[i].free_list.init ();
		zone_list[i].count = 0;
	}
}

static inline int
page_zone (Page *page)
{
	return phys_to_zone (page_to_phys (page));
}

void
dump_pgalloc_stats (void)
{
	size_t nfree;
	size_t zone_nfree[num_page_zones];

	{
		scoped_spinlock g (freelist_lock, IRQL_DISPATCH);
		nfree = total_free_pages;
		for (int i = 0; i < num_page_zones; i++)
			zone_nfree[i] = zone_list[i].count;
	}

	printk (PR_NOTICE "page_alloc:  %zu pages  (%zu MiB)  free\n",
			nfree, (nfree * PAGE_SIZE) / 1048576);
	for (int i = 0; i < num_page_zones; i++) {
		printk (PR_NOTICE ".. zone %d:  %zu pages  (%zu KiB)\n",
				i, zone_nfree[i],
				(zone_nfree[i] * PAGE_SIZE) / 1024);
	}
}

Page *
alloc_page (allocation_class aclass)
{
	Page *page;
	int zone = allocation_zone (aclass);

	{
		scoped_spinlock g (freelist_lock, IRQL_DISPATCH);

		for (;;) {
			if (!zone_list[zone].free_list.empty ()) [[likely]] {
				page = zone_list[zone].free_list.pop_front ();
				zone_list[zone].count--;
				total_free_pages--;
				break;
			}
			if (!zone_has_fallback (zone))
				return nullptr; // womp womp
			zone = fallback_zone (zone);
		}
	}

	if (aclass & __ALLOC_ZERO)
		memset ((void *) page_to_virt (page), 0, PAGE_SIZE);

	return page;
}

void
free_page (Page *page)
{
	page->flags = 0;
	int zone = page_zone (page);

	scoped_spinlock g (freelist_lock, IRQL_DISPATCH);

	zone_list[zone].free_list.push_front (page);
	zone_list[zone].count++;
	total_free_pages++;
}
