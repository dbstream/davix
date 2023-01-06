/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_MM_TYPES_H
#define __DAVIX_MM_TYPES_H

#include <davix/types.h>
#include <davix/spinlock.h>

struct slab_alloc;

struct page {
	unsigned long flags;
	union {
		struct {
			struct list list;
			unsigned order;
		} buddy;
		struct {
			union {
				/*
				 * Depending on whether or not this is the first
				 * page, we either store a pointer to the first
				 * page of the slab or the information needed to
				 * manage the slab.
				 */
				struct {
					struct slab_alloc *allocator;
					struct list slab_list;
					void *ob_head;
					unsigned long ob_count;
				};
				struct page *head;
			};
		} slab;
		struct {
			/*
			 * This is a page table page.
			 */

			/*
			 * List of pages to free, which may still be in the TLB
			 * and therefore must be delayed.
			 */
			struct list delayed_free_list;

			/*
			 * Page table page refcount.
			 *
			 * Child PTEs reference their parent page table pages.
			 */
			unsigned long refcnt;
		} pte;
		unsigned long _pad[7];
	};
};

static_assert(sizeof(struct page) == 8 * sizeof(unsigned long),
	"sizeof(struct page)");

/* 
 * Are we in the buddy system?
 */
#define PAGE_IN_FREELIST (1UL << 0)

/*
 * Are we the first slab page?
 */
#define PAGE_SLAB_HEAD (1UL << 1)

struct mm {
	void *page_tables;

	/*
	 * This spinlock must be held when using pgop_alloc_pte()
	 * and pgop_free_pte() functions.
	 *
	 * When using pgop_set_pte() and pgop_clear_pte() functions,
	 * it is enough to hold the lock on the vm_area.
	 */
	spinlock_t mm_lock;
};

#endif /* __DAVIX_MM_TYPES_H */
