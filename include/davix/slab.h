/**
 * Slab allocation and operations on slab pages.
 * Copyright (C) 2024  dbstream
 *
 * Slab allocators are best-suited for allocations that are smaller than half
 * the PAGE_SIZE. For anything bigger than half the PAGE_SIZE, it is better to
 * directly use the page allocator.
 *
 * NOTE on object alignment:
 *   - objects will be aligned to the greatest power-of-two less than PAGE_SIZE
 *     that divides objsize.
 */
#ifndef DAVIX_SLAB_H
#define DAVIX_SLAB_H 1

/* 'struct slab' is an opaque object */

struct pfn_entry;
struct slab;

/* Dump information about all slabs. */
extern void
slab_dump (void);

extern struct slab *
slab_create (const char *name, unsigned long objsize);

extern void *
slab_alloc (struct slab *slab);

extern void
slab_free (void *);

#endif /* DAVIX_SLAB_H */
