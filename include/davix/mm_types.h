/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_MM_TYPES_H
#define __DAVIX_MM_TYPES_H

struct page;
struct vm_area;
struct mm;

struct slab_alloc;

#include <davix/types.h>
#include <davix/spinlock.h>

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

struct vm_area {
	/*
	 * The range this VMA spans. Inclusive of start, but not end.
	 */
	unsigned long start;
	unsigned long end;

	/*
	 * Hugepage shift.
	 */
	unsigned hugepage_shift;

	/*
	 * VMAs are stored as AVL trees, but a linked list is used for fast
	 * access to the previous and next VMAs.
	 */
	struct avlnode avl;
	struct list list;

	/*
	 * Defined as:
	 *
	 *   max(max(biggest_gap_to_prev(left_subtree),
	 *           biggest_gap_to_prev(right_subtree),
	 *           ), start - prev_end)
	 *
	 * where biggest_gap_to_prev is:
	 *
	 *   vma ? (vma - prev_end) : 0
	 *
	 * and prev_end is:
	 *
	 *   prev ? prev->end : 0
	 */
	unsigned long biggest_gap_to_prev;

	/*
	 * When changing PTEs owned by this VMA, it is enough to hold this lock.
	 * See the comment for `mm->mm_lock` for information on lock ordering.
	 */
	spinlock_t vma_lock;

	/*
	 * The MM object we belong to.
	 */
	struct mm *mm;
};

#define vma_of(x) container_of((x), struct vm_area, avl)

struct mm {
	void *page_tables;

	/*
	 * VMAs are stored in both an AVL tree and a linked list.
	 */
	struct avltree vma_tree;
	struct list vma_list;

	/*
	 * This spinlock protects the page tables and the VMA tree.
	 *
	 * In order to lock a VMA, you must hold this lock.
	 *
	 * If you are only modifying existing PTEs (i.e. those owned by a VMA),
	 * you only need to hold the VMA lock, so for page faults (and other PTE
	 * modifications), the code should look like this (pseudo-code):
	 *
	 *   lock(mm);
	 *   vma = find_vma(mm, addr);
	 *   lock(vma);
	 *   unlock(mm);
	 *   pgop_set_pte(addr, new value);
	 *   unlock(vma);
	 *
	 * NOTE: pgop_end() must come before unlock(vma);
	 */
	spinlock_t mm_lock;

	refcnt_t refcnt;
};

typedef enum pgcachemode {
	PG_WRITEBACK,		/* Normal, "cacheable" memory. Use this when possible. */
#ifdef CONFIG_HAVE_PGCACHEMODE_WRITETHROUGH
	PG_WRITETHROUGH,	/* Cache reads but not writes. */
#endif
#ifdef CONFIG_HAVE_PGCACHEMODE_WRITECOMBINING
	PG_WRITECOMBINE,	/* Don't cache anything, but do combine consecutive writes. */
#endif
#ifdef CONFIG_HAVE_PGCACHEMODE_UNCACHED_MINUS
	PG_UNCACHED_MINUS,	/* Same as uncached, but can be overridden by platform. */
#endif
	PG_UNCACHED,		/* Don't cache anything. */
	NUM_PG_CACHEMODES
} pgcachemode_t;

/*
 * Fall-back to PG_UNCACHED if arch doesn't provide one of the above.
 */
#ifndef CONFIG_HAVE_PGCACHEMODE_WRITETHROUGH
#define PG_WRITETHROUGH PG_UNCACHED
#endif

#ifndef CONFIG_HAVE_PGCACHEMODE_WRITECOMBINING
#define PG_WRITECOMBINING PG_UNCACHED
#endif

#ifndef CONFIG_HAVE_PGCACHEMODE_UNCACHED_MINUS
#define PG_UNCACHED_MINUS PG_UNCACHED
#endif

typedef bitwise unsigned long pgmode_t;

#define PG_READABLE	((force pgmode_t) (1 << 0))	/* PROT_READ */
#define PG_WRITABLE	((force pgmode_t) (1 << 1))	/* PROT_WRITE */
#define PG_EXECUTABLE	((force pgmode_t) (1 << 2))	/* PROT_EXEC */

#endif /* __DAVIX_MM_TYPES_H */
