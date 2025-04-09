/**
 * Page allocation.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/page.h>
#include <davix/printk.h>
#include <davix/spinlock.h>
#include <string.h>

static PageList global_free_list;
static size_t total_free_pages;

static spinlock_t freelist_lock;

void
dump_pgalloc_stats (void)
{
	size_t nfree;

	{
		scoped_spinlock (freelist_lock, IRQL_DISPATCH);
		nfree = total_free_pages;
	}

	printk (PR_NOTICE "page_alloc:  %zu pages  (%zu MiB)  free\n",
			nfree, (nfree * PAGE_SIZE) / 1048576);
}

Page *
alloc_page (allocation_class aclass)
{
	Page *page;

	{
		scoped_spinlock (freelist_lock, IRQL_DISPATCH);

		if (global_free_list.empty ())
			return nullptr; // womp womp

		page = global_free_list.pop_front ();
		total_free_pages--;
	}

	if (aclass & __ALLOC_ZERO)
		memset ((void *) page_to_virt (page), 0, PAGE_SIZE);

	return page;
}

void
free_page (Page *page)
{
	scoped_spinlock (freelist_lock, IRQL_DISPATCH);

	global_free_list.push_front (page);
	total_free_pages++;
}
