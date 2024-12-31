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

extern void *
kmalloc (unsigned long size);

extern void
kfree (void *ptr);

#endif /** _DAVIX_KMALLOC_H  */
