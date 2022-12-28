/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_SLAB_H
#define __DAVIX_SLAB_H

#include <davix/types.h>
#include <davix/spinlock.h>

struct slab_alloc {
	/*
	 * A list of slabs (``struct page``s) in this allocator.
	 */
	struct list slab_list;

	/*
	 * Some statistics that can be printed to the console.
	 */
	unsigned long total_slabs;
	unsigned long total_obs;
	unsigned long free_obs;

	spinlock_t lock;

	/*
	 * Things that we write to during slab_init(), but which are
	 * only read when the slab is alive.
	 */
	unsigned long ob_size;
	unsigned long obs_per_slab;
	unsigned slab_order;
	alloc_flags_t alloc_flags;	/* passed to alloc_page() */
};

void slab_init(struct slab_alloc *alloc, unsigned long ob_size,
	alloc_flags_t flags);

void slab_destroy(struct slab_alloc *alloc);

void *slab_allocate(struct slab_alloc *alloc);
void slab_free(void *mem);

#endif /* __DAVIX_SLAB_H */
