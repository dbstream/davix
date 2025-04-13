/**
 * Slab allocator.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/kmalloc.h>
#include <davix/page.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/slab.h>
#include <davix/spinlock.h>
#include <dsl/align.h>
#include <string.h>
#include <vsnprintf.h>
#include <new>

bool
ptr_is_slab (const void *ptr)
{
	Page *page = virt_to_page ((uintptr_t) ptr);
	return (page->flags & PAGE_SLAB) == PAGE_SLAB;
}

struct SlabAllocator {
	spinlock_t lock;
	size_t nr_full;
	size_t nr_partial;
	PageList page_full;
	PageList page_partial;
	size_t nr_empty;
	size_t nfree;

	size_t inp_obj_size;
	size_t inp_obj_align;
	size_t real_obj_size;
	size_t objs_per_page;
	dsl::ListHead listHead;
	char name[32];
};

static void *
wrap (SlabAllocator *allocator, allocation_class aclass, void *ptr)
{
	ptr = new (ptr) unsigned char [allocator->inp_obj_size];
	if (aclass & __ALLOC_ZERO)
		memset (ptr, 0, allocator->inp_obj_size);
	return ptr;
}

void *
slab_alloc (SlabAllocator *allocator, allocation_class aclass)
{
	aclass = ALLOC_KERNEL | (aclass & __ALLOC_HIGHPRIO);

	scoped_spinlock g (allocator->lock, IRQL_DISPATCH);

	Page *page = nullptr;
	if (allocator->nr_full) {
		page = allocator->page_full.pop_front ();
		allocator->nr_full--;
	} else if (allocator->nr_partial) {
		page = allocator->page_partial.pop_front ();
		allocator->nr_partial--;
	}

	if (page) {
		void *obj = page->slab.pobj;
		page->slab.pobj = *(void **) obj;
		if (!--(page->slab.nfree))
			allocator->nr_empty++;
		else {
			allocator->nr_partial++;
			allocator->page_partial.push_front (page);
		}

		allocator->nfree--;
		return wrap (allocator, aclass, obj);
	}

	page = alloc_page (aclass);
	if (!page)
		return nullptr;
	page->flags = PAGE_SLAB;
	page->slab.allocator = allocator;
	page->slab.nfree = allocator->objs_per_page - 1;
	void **head = &page->slab.pobj;

	uintptr_t addr = page_to_virt (page);
	for (size_t i = 1; i < allocator->objs_per_page; i++) {
		void *obj = (void *) (addr + i * allocator->real_obj_size);
		*head = obj;
		head = new (obj) void *;
	}
	*head = nullptr;
	allocator->nfree += allocator->objs_per_page - 1;
	allocator->nr_partial++;
	allocator->page_partial.push_front (page);
	return wrap (allocator, aclass, (void *) addr);
}

void
slab_free (void *ptr)
{
	Page *page = virt_to_page ((uintptr_t) ptr);
	SlabAllocator *allocator = page->slab.allocator;

	{
		scoped_spinlock g (allocator->lock, IRQL_DISPATCH);

		void **head = new (ptr) void *;
		*head = page->slab.pobj;
		page->slab.pobj = ptr;
		page->slab.nfree++;
		if (page->slab.nfree == 1) {
			allocator->nr_partial++;
			allocator->page_partial.push_front (page);
			return;
		} else if (page->slab.nfree != allocator->objs_per_page)
			return;

		page->node.remove ();
		allocator->nr_partial--;
		if (allocator->nr_full < 1) {
			allocator->nr_full++;
			allocator->page_full.push_front (page);
			return;
		}

		allocator->nfree -= allocator->objs_per_page;
	}

	free_page (page);
}

static dsl::TypedList<SlabAllocator, &SlabAllocator::listHead> globalSlabList;
static spinlock_t globalSlabSpinlock;

static SlabAllocator slab_allocator;

static void
dump_one (SlabAllocator *allocator)
{
	size_t obj_size = allocator->real_obj_size;
	size_t per_page = allocator->objs_per_page;
	size_t nr_full, nr_partial, nr_empty, nfree;
	{
		scoped_spinlock g (allocator->lock, IRQL_DISPATCH);
		nr_full = allocator->nr_full;
		nr_partial = allocator->nr_partial;
		nr_empty = allocator->nr_empty;
		nfree = allocator->nfree;
	}

	printk (PR_INFO ".. %-16s %4zu %3zu   %4zu %4zu %4zu  %4zu %4zu\n",
			allocator->name,
			obj_size, per_page,
			nr_full, nr_partial, nr_empty,
			per_page * (nr_full + nr_partial + nr_empty),
			nfree);
}

void
slab_dump (void)
{
	printk (PR_INFO "Slab allocators:\n");
	printk (PR_INFO ".. name            size perpg full part empt  ntot nfree\n");
	dump_one (&slab_allocator);
	scoped_spinlock g (globalSlabSpinlock, IRQL_DISPATCH);
	for (SlabAllocator *allocator : globalSlabList)
		dump_one (allocator);
}

static void
init_new_allocator (SlabAllocator *allocator, const char *name,
		size_t inp_obj_size, size_t inp_obj_align,
		size_t real_obj_size)
{
	allocator->lock.init ();
	allocator->nr_full = 0;
	allocator->nr_partial = 0;
	allocator->page_full.init ();
	allocator->page_partial.init ();
	allocator->nr_empty = 0;
	allocator->nfree = 0;
	allocator->inp_obj_size = inp_obj_size;
	allocator->inp_obj_align = inp_obj_align;
	allocator->real_obj_size = real_obj_size;
	allocator->objs_per_page = PAGE_SIZE / real_obj_size;
	strncpy (allocator->name, name, sizeof (allocator->name));
	allocator->name[sizeof (allocator->name) - 1] = 0;
}

SlabAllocator *
slab_create (const char *name, size_t size, size_t align)
{
	if (align & (align - 1)) {
		printk (PR_ERROR "slab_create():  align=%zu is not a power-of-two!\n",
				align);
		return nullptr;
	}

	if (size > PAGE_SIZE / 2 || align > PAGE_SIZE / 2) {
		printk (PR_ERROR "slab_create():  requested object size is too large for slab allocation!\n");
		return nullptr;
	}

	if (!size) {
		printk (PR_ERROR "slab_create():  requested size of zero (0) bytes is invalid!\n");
		return nullptr;
	}

	if (sizeof (void *) > align)
		align = sizeof (void *);
	size_t realsize = dsl::align_up (size, align);
	SlabAllocator *allocator = (SlabAllocator *) slab_alloc (&slab_allocator, ALLOC_KERNEL);
	if (!allocator)
		return nullptr;

	init_new_allocator (allocator, name, size, align, realsize);

	scoped_spinlock g (globalSlabSpinlock, IRQL_DISPATCH);
	globalSlabList.push_back (allocator);

	return allocator;
}

static SlabAllocator *kmalloc_slabs[9];
static size_t kmalloc_sizes[9] = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048 };

void *
kmalloc (size_t size, allocation_class aclass)
{
	for (int i = 0; i < 9; i++) {
		if (kmalloc_sizes[i] >= size)
			return slab_alloc (kmalloc_slabs[i], aclass);
	}

	return nullptr;
}

void
kfree (void *ptr)
{
	if (!ptr) [[unlikely]] {
		return;
	}

	slab_free (ptr);
}

void
kmalloc_init (void)
{
	init_new_allocator (&slab_allocator, "SlabAllocator",
			sizeof (SlabAllocator), sizeof (void *),
			dsl::align_up (sizeof (SlabAllocator), 8 * sizeof (void *)));

	for (int i = 0; i < 9; i++) {
		char name[32];
		size_t size = kmalloc_sizes[i];
		snprintf (name, sizeof (name), "kmalloc-%zu", size);
		kmalloc_slabs[i] = slab_create (name, size, size);
		if (!kmalloc_slabs[i])
			panic ("init_kmalloc:  failed to create %s", name);
	}
}
