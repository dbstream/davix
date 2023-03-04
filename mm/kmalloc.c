/* SPDX-License-Identifier: MIT */
#include <davix/kmalloc.h>
#include <davix/slab.h>
#include <davix/printk.h>

static struct slab_alloc kmalloc_slabs[16];

void init_kmalloc(void)
{
#define S(i, sz) slab_init(&kmalloc_slabs[i], sz, ALLOC_KERNEL)
	S(0, 8);
	S(1, 16);
	S(2, 24);
	S(3, 32);
	S(4, 48);
	S(5, 64);
	S(6, 96);
	S(7, 128);
	S(8, 192);
	S(9, 256);
	S(10, 384);
	S(11, 512);
	S(12, 768);
	S(13, 1024);
	S(14, 2048);
	S(15, 3072);
#undef S
}

void dump_kmalloc_slabs(void)
{
	debug("Kmalloc slabs:\n");
	debug("  <slab>       <total>  <allocated> <free> <slabs> <objs/slab> <order>\n");
	for(int i = 0; i <16; i++) {
		struct slab_alloc *slab = &kmalloc_slabs[i];
		unsigned long sz, total, totfree, nslab, slab_objs, slab_order;

		sz = slab->ob_size;
		slab_objs = slab->obs_per_slab;
		slab_order = slab->slab_order;
		int irqflag = spin_acquire_irq(&slab->lock);
		total = slab->total_obs;
		totfree = slab->free_obs;
		nslab = slab->total_slabs;
		spin_release_irq(&slab->lock, irqflag);
		debug("  kmalloc-%-4lu %11lu %7lu %7lu %7lu %11lu %lu\n",
			sz, total, total - totfree, totfree, nslab, slab_objs, slab_order);
	}
}

void *kmalloc(unsigned long size)
{
	if(size == 0) {
		warn("kmalloc: attempt to allocate with size 0. Caller: %p\n",
			RET_ADDR);
		return NULL;
	}

	if(size > 3072) {
		warn("kmalloc: attempt to allocate with size %lu (> 3072). Caller: %p\n",
			size, RET_ADDR);
		return NULL;
	}

	struct slab_alloc *alloc = &kmalloc_slabs[0];
	while(alloc->ob_size < size)
		alloc++;

	return slab_allocate(alloc);
}

void kfree(void *mem)
{
	if(!mem) {
		warn("kmalloc: kfree(NULL) called, which is deprecated. Caller: %p\n",
			RET_ADDR);
		return;
	}

	slab_free(mem);
}
