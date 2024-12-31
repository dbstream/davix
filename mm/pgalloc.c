/**
 * Page allocation and deallocation.
 * Copyright (C) 2024  dbstream
 */
#include <davix/atomic.h>
#include <davix/list.h>
#include <davix/page.h>
#include <davix/printk.h>
#include <davix/spinlock.h>

static spinlock_t pgalloc_lock;

static unsigned long total_free_pages;
static unsigned long total_reserved_pages;

void
pgalloc_dump (void)
{
	bool flag = spin_lock_irq (&pgalloc_lock);
	unsigned long num_free = total_free_pages;
	unsigned long num_reserved = total_reserved_pages;
	spin_unlock_irq (&pgalloc_lock, flag);

	unsigned long bytes_free = PAGE_SIZE * num_free;
	unsigned long bytes_resv = PAGE_SIZE * num_reserved;

	unsigned long kib_free = bytes_free / 1024UL;
	unsigned long kib_resv = bytes_resv / 1024UL;

	printk (PR_NOTICE "pgalloc: %lu KiB free, %lu KiB reserved\n", kib_free, kib_resv);
}

/**
 * Keep a number of pages reserved for kernel-internal allocations and to
 * avoid true OOM.
 */
#define RESERVED_KERNEL_PAGES	1024

static DEFINE_EMPTY_LIST (free_list);

static errno_t
__reserve_pages (unsigned long num)
{
	unsigned long x = total_reserved_pages + num;
	if (x < total_reserved_pages)
		/** overflow */
		return ENOMEM;

	if (x + RESERVED_KERNEL_PAGES < x)
		/** overflow */
		return ENOMEM;

	x += RESERVED_KERNEL_PAGES;
	if (x > total_free_pages)
		/** not enough free pages */
		return ENOMEM;

	total_reserved_pages += num;
	return ESUCCESS;
}

static void
__unreserve_pages (unsigned long num)
{
	total_reserved_pages -= num;
}

static struct pfn_entry *
__alloc_page (alloc_flags_t flags)
{
	if (list_empty (&free_list))
		return NULL;

	if (flags & __ALLOC_RESERVED) {
		total_reserved_pages--;
	} else if (flags & __ALLOC_ACCT) {
		if (total_reserved_pages + RESERVED_KERNEL_PAGES >= total_free_pages)
			return NULL;
	}

	total_free_pages--;
	struct pfn_entry *page = list_item (struct pfn_entry, pfn_free.list, free_list.next);
	list_delete (&page->pfn_free.list);
	return page;
}

static void
__free_page (struct pfn_entry *page, alloc_flags_t flags)
{
	if (flags & __ALLOC_RESERVED)
		total_reserved_pages++;

	list_insert (&free_list, &page->pfn_free.list);
	total_free_pages++;
}

struct pfn_entry *
alloc_page (alloc_flags_t flags)
{
	bool flag = spin_lock_irq (&pgalloc_lock);
	struct pfn_entry *page = __alloc_page (flags);
	spin_unlock_irq (&pgalloc_lock, flag);

	return page;
}

void
free_page (struct pfn_entry *page, alloc_flags_t flags)
{
	bool flag = spin_lock_irq (&pgalloc_lock);
	__free_page (page, flags);
	spin_unlock_irq (&pgalloc_lock, flag);
}

errno_t
reserve_pages (unsigned long num)
{
	bool flag = spin_lock_irq (&pgalloc_lock);
	errno_t result = __reserve_pages (num);
	spin_unlock_irq (&pgalloc_lock, flag);
	return result;
}

void
unreserve_pages (unsigned long num)
{
	bool flag = spin_lock_irq (&pgalloc_lock);
	__unreserve_pages (num);
	spin_unlock_irq (&pgalloc_lock, flag);
}
