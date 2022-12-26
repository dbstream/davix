/* SPDX-License-Identifier: MIT */
#include <davix/list.h>
#include <davix/mm.h>
#include <davix/page_alloc.h>
#include <davix/printk.h>
#include <davix/printk_lib.h>
#include <asm/page.h>

static struct page *__get_from_freelist(struct page_freelist *list)
{
	if(list_empty(&list->pages))
		return NULL;

	struct page *page = container_of(list->pages.next,
		struct page, buddy.list);
	list_del(&page->buddy.list);
	list->count--;
	clear_page_bit(page, PAGE_IN_FREELIST);
	return page;
}

static void __add_to_freelist(struct page_freelist *list,
	struct page *page)
{
	set_page_bit(page, PAGE_IN_FREELIST);
	list->count++;
	list_add(&list->pages, &page->buddy.list);
}

static void __take_off_freelist(struct page_freelist *list,
	struct page *page)
{
	list->count--;
	list_del(&page->buddy.list);
	clear_page_bit(page, PAGE_IN_FREELIST);
}

static inline struct page *buddy_of(struct page *page, unsigned order)
{
	return pfn_to_page(page_to_pfn(page) ^ (1UL << order));
}

static struct page *alloc_page_from_freelist(struct page_alloc_zone *zone,
	unsigned order)
{
	struct page *page = NULL;
	unsigned int current_order = order;
	spin_acquire(&zone->lock);

	for(;;) {
		if(current_order >= NUM_ORDERS) {
			spin_release(&zone->lock);
			return NULL;
		}

		page = __get_from_freelist(&zone->freelists[current_order]);
		if(page)
			break;
		
		current_order++;
	}

	while(current_order > order) {
		current_order--;
		struct page *buddy = buddy_of(page, current_order);
		buddy->buddy.order = current_order;
		__add_to_freelist(&zone->freelists[current_order], buddy);
	}

	spin_release(&zone->lock);
	return page;
}

static void free_page_to_freelist(struct page *page,
	struct page_alloc_zone *zone, unsigned order)
{
	spin_acquire(&zone->lock);

	for(;;) {
		if(order == NUM_ORDERS - 1)
			break;

		struct page *buddy = buddy_of(page, order);
		if(zone != page_to_zone(buddy))
			break;
		if(!test_page_bit(buddy, PAGE_IN_FREELIST))
			break;
		if(buddy->buddy.order != order)
			break;

		__take_off_freelist(&zone->freelists[order], buddy);
		if(page_to_pfn(buddy) < page_to_pfn(page))
			swap(page, buddy);

		order++;
	}

	page->buddy.order = order;
	__add_to_freelist(&zone->freelists[order], page);

	spin_release(&zone->lock);
}

struct page *alloc_page(alloc_flags_t flags, unsigned order)
{
	struct page *page;

	if(order >= NUM_ORDERS) {
		warn("mm: Cannot allocate pages of order %u. Caller: %p\n",
			order, RET_ADDR);
		return NULL;
	}

	struct page_alloc_zone *zone = alloc_flags_to_zone(flags);
	while(zone) {
		page = alloc_page_from_freelist(zone, order);
		if(page)
			goto out;
		zone = zone->fallback;
	}
	return NULL;
out:
	if(flags & __ALLOC_ZERO)
		memset((void *) page_to_virt(page), 0, PAGE_SIZE << order);

	return page;
}

void free_page(struct page *page, unsigned order)
{
	if(order >= NUM_ORDERS) {
		error("mm: Cannot free pages of order %u. Caller: %p\n",
			order, RET_ADDR);
		return;
	}

	struct page_alloc_zone *zone = page_to_zone(page);
	free_page_to_freelist(page, zone, order);
}

void dump_pgalloc_info(void)
{
	char buf[128];
	info("Buddy system:\n");
	for_each_zone(zone) {
		buf[0] = 0;
		for(unsigned i = 0; i < NUM_ORDERS; i++) {
			snprintk(buf, 128, "%s %4lu",
				buf, zone->freelists[i].count);
		}
		info("  %-10s%s\n", zone->zone_name, buf);
	}
}
