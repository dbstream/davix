/**
 * Kernel malloc().
 * Copyright (C) 2024  dbstream
 *
 * kmalloc() is implemented in terms of vmap_pages for large (>PAGE_SIZE) sizes,
 * directly via the page allocator for PAGE_SIZEd allocations, and via slab for
 * small (<=PAGE_SIZE/2) allocations.
 */
#ifndef _DAVIX_KMALLOC_H
#define _DAVIX_KMALLOC_H 1

#include <asm/page_defs.h>

extern void
kfree (void *ptr);

static unsigned long kmalloc_slab_sizes[16] = {
	8, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048
};

static inline int
kmalloc_find_slab (unsigned long size)
{
	for (int i = 0; i < 16; i++)
		if (kmalloc_slab_sizes[i] >= size)
			return i;

	__builtin_unreachable ();
}

extern void *
kmalloc_known_slab (int slab_idx);

extern void *
kmalloc_vmap (unsigned long nr_pages);

extern void *
kmalloc_normal (unsigned long size);

static inline void *
kmalloc (unsigned long size)
{
	if (__builtin_constant_p (size)) {
		if (!size)
			return 0;

		if (size <= kmalloc_slab_sizes[15])
			return kmalloc_known_slab (kmalloc_find_slab (size));
	}

	return kmalloc_normal (size);
}

extern void
kmalloc_init (void);

#endif /** _DAVIX_KMALLOC_H  */
