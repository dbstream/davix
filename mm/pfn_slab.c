/**
 * Slab allocation and operations on slab pages.
 * Copyright (C) 2024  dbstream
 */
#include <davix/align.h>
#include <davix/initmem.h>
#include <davix/page.h>
#include <davix/printk.h>
#include <davix/slab.h>
#include <davix/string.h>

struct slab {
	spinlock_t lock;

	/**
	 * Full, partial, and empty pages.
	 *
	 * A full page is a page for which all objects are free. For partial
	 * pages, some objects are free, and for empty pages, no objects are free.
	 */
	unsigned long nr_full;
	unsigned long nr_partial;
	unsigned long nr_empty;

	struct list pg_full_list;
	struct list pg_partial_list;

	/**
	 * Per-slab statistics.
	 */
	unsigned long nr_obj_free;

	/**
	 * obj_size is the value passed to slab_create. real_obj_size is what
	 * is actually available for each allocation. objs_per_page is how many
	 * objects fit into one page.
	 */
	unsigned long obj_size;
	unsigned long real_obj_size;
	unsigned long objs_per_page;

	struct list slab_list;

	/**
	 * Symbolic name of this slab. Not null-terminated.
	 * NOTE: this is not a unique identifier for the slab.
	 */
	char name[16];
};

/**
 * Keep a list of all slabs for debugging purposes.
 */

static DEFINE_EMPTY_LIST (slab_list);
static spinlock_t slab_list_lock;

/**
 * New opaque slab handles are themselves allocated from a slab. Use a static
 * variable for this purpose.
 */

static struct slab static_slab;

static void
slab_dump_one (struct slab *slab)
{
	unsigned long nr_full = atomic_load_relaxed (&slab->nr_full);
	unsigned long nr_partial = atomic_load_relaxed (&slab->nr_partial);
	unsigned long nr_empty = atomic_load_relaxed (&slab->nr_empty);
	printk (PR_INFO "  %16.16s %4lu %4lu %4lu  %4lu %4lu %4lu  %4lu %4lu\n",
		slab->name,
		slab->obj_size, slab->real_obj_size, slab->objs_per_page,
		nr_full, nr_partial, nr_empty,
		slab->objs_per_page * (nr_full + nr_partial + nr_empty),
		atomic_load_relaxed (&slab->nr_obj_free));
}

void
slab_dump (void)
{
	/**
	 * FIXME: when we introduce a way to destroy slabs, remember to protect
	 * this function using something like RCU. Currently it is safe, because
	 * we never delete anything from the slab_list.
	 */

	printk (PR_INFO "Slab allocators:\n");
	printk (PR_INFO "              name size/real/page  full/part/empt  totl/free\n");

	list_for_each (entry, &slab_list)
		slab_dump_one (list_item (struct slab, slab_list, entry));
}

static void
slab_init_new (struct slab *slab, const char *name, unsigned long obj_size)
{
	spinlock_init (&slab->lock);
	slab->nr_full = 0;
	slab->nr_partial = 0;
	slab->nr_empty = 0;
	list_init (&slab->pg_full_list);
	list_init (&slab->pg_partial_list);
	slab->nr_obj_free  = 0;
	slab->obj_size = obj_size;

	obj_size = sizeof (long) > obj_size ? sizeof (long) : obj_size;

	/* compute the index of the most significant bit of obj_size */
	int msb = 1;
	for (; obj_size >> (msb + 1); msb++);

	/**
	 * allow sizes like 8, 16, 24, 32, 48, 64, 96...
	 * allow those that are equal to 1 or 3 left-shifted by some amount
	 * disallow sizes like 6 and 12 (not aligned to sizeof (long))
	 */
	unsigned long real_size = ALIGN_UP (obj_size, 1UL << (msb - 1));
	real_size = ALIGN_UP (real_size, sizeof (long));

	slab->real_obj_size = real_size;
	slab->objs_per_page = PAGE_SIZE / real_size;

	strncpy (slab->name, name, sizeof (slab->name));

	bool flag = spin_lock_irq (&slab_list_lock);
	list_insert_back (&slab_list, &slab->slab_list);
	spin_unlock_irq (&slab_list_lock, flag);
}

/**
 * Provide wrappers for alloc_page and free_page that work when mm_is_early.
 */

static struct pfn_entry *
slab_alloc_page (void)
{
	if (mm_is_early) {
		unsigned long addr = initmem_alloc_phys (PAGE_SIZE, PAGE_SIZE);
		if (!addr)
			return NULL;
		return phys_to_pfn_entry (addr);
	}

	return alloc_page (ALLOC_KERNEL);
}

static void
slab_free_page (struct pfn_entry *page)
{
	set_page_flags (page, 0);
	if (mm_is_early) {
		initmem_free_phys (pfn_entry_to_phys (page), PAGE_SIZE);
		return;
	}
	free_page (page, ALLOC_KERNEL);
}

static struct pfn_entry *
slab_new_page (unsigned long obj_size, unsigned long objs_per_page)
{
	struct pfn_entry *page = slab_alloc_page ();
	if (!page)
		return NULL;

	set_page_flags (page, PFN_SLAB);

	spinlock_init (&page->pfn_slab.lock);
	page->pfn_slab.nfree = objs_per_page;
	page->pfn_slab.pobj = NULL;
	while (objs_per_page--) {
		void *p = (void *)
			(pfn_entry_to_virt (page) + objs_per_page * obj_size);
		*(void **) p = page->pfn_slab.pobj;
		page->pfn_slab.pobj = p;
	}

	return page;
}

/* Allocate an object from a slab. */
void *
slab_alloc (struct slab *slab)
{
	void *ret = NULL;
	bool flag = spin_lock_irq (&slab->lock);
	struct pfn_entry *page;

	/* Try grabbing from the pg_partial_list first. */
	if (slab->nr_partial) {
		page = list_item (struct pfn_entry,
			pfn_slab.list, slab->pg_partial_list.next);

		ret = page->pfn_slab.pobj;
		page->pfn_slab.pobj = *(void **) ret;

		if (!--page->pfn_slab.nfree) {
			/* move the page from partial to empty */
			list_delete (&page->pfn_slab.list);
			slab->nr_partial--;
			slab->nr_empty++;
		}

		goto out;
	}

	/* That didn't work, now look at pg_full_list. */
	if (slab->nr_full) {
		page = list_item (struct pfn_entry,
			pfn_slab.list, slab->pg_full_list.next);

		ret = page->pfn_slab.pobj;
		page->pfn_slab.pobj = *(void **) ret;

		page->pfn_slab.nfree--;

		/**
		 * Move the page from full to partial.
		 * A slab page cannot transition directly from full to empty, as
		 * every slab page is guaranteed to fit at least two objects.
		 */
		list_delete (&page->pfn_slab.list);
		list_insert (&slab->pg_partial_list, &page->pfn_slab.list);
		slab->nr_full--;
		slab->nr_partial++;

		goto out;
	}

	/* We have to allocate a new page. */
	page = slab_new_page (slab->real_obj_size, slab->objs_per_page);
	if (page) {
		/* Accounting */
		page->pfn_slab.slab = slab;
		slab->nr_obj_free += slab->objs_per_page;

		ret = page->pfn_slab.pobj;
		page->pfn_slab.pobj = *(void **) ret;
		page->pfn_slab.nfree--;

		list_insert (&slab->pg_partial_list, &page->pfn_slab.list);
		slab->nr_partial++;
	}
out:
	if (ret)
		slab->nr_obj_free--;
	spin_unlock_irq (&slab->lock, flag);
	return ret;
}

/* Free an object back to the slab. */
void
slab_free (void *p)
{
	struct pfn_entry *page = virt_to_pfn_entry (p);
	struct slab *slab = page->pfn_slab.slab;

	bool flag = spin_lock_irq (&slab->lock);

	*(void **) p = page->pfn_slab.pobj;
	page->pfn_slab.pobj = p;

	slab->nr_obj_free++;

	unsigned long nfree = ++page->pfn_slab.nfree;
	if (nfree == 1) {
		list_insert (&slab->pg_partial_list, &page->pfn_slab.list);
		slab->nr_empty--;
		slab->nr_partial++;
	} else if (nfree == slab->objs_per_page) {
		slab->nr_partial--;
		list_delete (&page->pfn_slab.list);
		if (slab->nr_full >= 2) {
			/* don't overfill the slab */
			slab_free_page (page);
			slab->nr_obj_free -= slab->objs_per_page;
		} else {
			list_insert (&slab->pg_full_list, &page->pfn_slab.list);
			slab->nr_full++;
		}
	}

	spin_unlock_irq (&slab->lock, flag);
}

static int slab_initialized;
static spinlock_t slab_init_mutex;

static inline void
slab_inited (void)
{
	if (atomic_load_acquire (&slab_initialized))
		return;

	bool flag = spin_lock_irq (&slab_init_mutex);
	if (!slab_initialized) {
		slab_init_new (&static_slab, "slab", sizeof (struct slab));
		atomic_store_release (&slab_initialized, 1);
	}
	spin_unlock_irq (&slab_init_mutex, flag);
}

struct slab *
slab_create (const char *name, unsigned long objsize)
{
	if (!objsize || objsize > (PAGE_SIZE / 2)) {
		/**
		 * A zero object size is invalid, as is an object size larger
		 * than half the page size.
		 */
		printk (PR_WARN "slab: Attempt to create slab %s with object size %lu\n",
			name, objsize);
		return NULL;
	}

	slab_inited ();
	struct slab *handle = slab_alloc (&static_slab);
	if (!handle)
		return NULL;

	slab_init_new (handle, name, objsize);
	return handle;
}
