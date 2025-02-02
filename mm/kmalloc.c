/**
 * Kernel malloc.
 * Copyright (C) 2024  dbstream
 *
 * kmalloc() is implemented in terms of vmap_pages for large (>PAGE_SIZE) sizes,
 * directly via the page allocator for PAGE_SIZEd allocations, and via slab for
 * small (<=PAGE_SIZE/2) allocations.
 */
#include <davix/align.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/initmem.h>
#include <davix/kmalloc.h>
#include <davix/page.h>
#include <davix/slab.h>
#include <davix/snprintf.h>
#include <davix/vmap.h>

static struct pfn_entry *
kmalloc_alloc_page (void)
{
	if (__builtin_expect (mm_is_early, 0))
		return virt_to_pfn_entry (initmem_alloc_virt (PAGE_SIZE, PAGE_SIZE));

	return alloc_page (ALLOC_KERNEL);
}

static void
kmalloc_free_page (struct pfn_entry *page)
{
	if (__builtin_expect (mm_is_early, 0))
		initmem_free_phys (pfn_entry_to_phys (page), PAGE_SIZE);
	else
		free_page (page, ALLOC_KERNEL);		
}

void
kfree_vmap (void *ptr)
{
	struct vmap_area *area = find_vmap_area (ptr);
	if (!area)
		// kfree_vmap was called incorrectly??
		return;

	struct pfn_entry **pages = area->u.pages;
	unsigned long nr_pages = (1 + area->vma_node.last - area->vma_node.first) / PAGE_SIZE;

	free_vmap_area (area);

	_Static_assert (VMAP_PAGES_USES_GUARD_PAGES == 1, "!VMAP_PAGES_USES_GUARD_PAGES; fix kfree_vmap");

	// Be careful! vmap_pages inserts guard pages.
	for (unsigned long i = 0; i < nr_pages - 2; i++)
		kmalloc_free_page (pages[i]);
	kfree (pages);
}

void
kfree (void *ptr)
{
	if (__builtin_expect (!ptr, 0))
		return;

	unsigned long addr = (unsigned long) ptr;
	if (addr >= KERNEL_VMAP_LOW && addr <= KERNEL_VMAP_HIGH) {
		kfree_vmap (ptr);
		return;
	}

	struct pfn_entry *page = virt_to_pfn_entry (ptr);
	if (get_page_flags (page) & PFN_SLAB)
		slab_free (ptr);
	else
		kmalloc_free_page (page);
}

_Static_assert (PAGE_SIZE == 4096, "PAGE_SIZE != 4096; fix kmalloc");
_Static_assert (sizeof (long) == 8, "sizeof(long) != 8; fix kmalloc");

static struct slab *slabs[16];

void
kmalloc_init (void)
{
	for (int i = 0; i < 16; i++){
		char name[16];
		snprintf (name, sizeof (name), "kmalloc-%lu", kmalloc_slab_sizes[i]);
		slabs[i] = slab_create (name, kmalloc_slab_sizes[i]);
		if (!slabs[i])
			panic ("kmalloc_init: slab_create returned NULL!");
	}
}

void *
kmalloc_known_slab (int slab_idx)
{
	return slab_alloc (slabs[slab_idx]);
}

void *
kmalloc_normal (unsigned long size)
{
	if (size <= 2048) {
		if (!size)
			return NULL;

		return slab_alloc (slabs[kmalloc_find_slab (size)]);
	}

	if (size <= PAGE_SIZE)
		return (void *) pfn_entry_to_virt (kmalloc_alloc_page ());

	unsigned long nr_pages = ALIGN_UP (size, PAGE_SIZE) / PAGE_SIZE;
	if (!nr_pages)
		return NULL;

	/** NB: recursion is bounded by size always getting smaller  */
	struct pfn_entry **pages = kmalloc (nr_pages * sizeof (struct pfn_entry *));
	if (!pages)
		return NULL;

	for (unsigned long i = 0; i < nr_pages; i++) {
		pages[i] = kmalloc_alloc_page ();
		if (!pages[i]) {
			while (i)
				kmalloc_free_page (pages[--i]);
			kfree (pages);
			return NULL;
		}
	}

	void *ptr = vmap_pages ("kmalloc", pages, nr_pages, PAGE_KERNEL_DATA);
	if (ptr)
		return ptr;

	for (unsigned long i = 0; i < nr_pages; i++)
		kmalloc_free_page (pages[i]);
	kfree (pages);
	return NULL;
}
