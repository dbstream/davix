/**
 * Kernel virtual memory manager.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_VMAP_H
#define _DAVIX_VMAP_H 1

#include <davix/vma_tree.h>
#include <asm/page_defs.h>

struct pfn_entry;

struct vmap_area {
	struct vma_node vma_node;
	union {
		struct pfn_entry **pages;
	} u;
	char purpose[24];
};

extern void
vmap_init (void);

extern void
vmap_dump (void);

/**
 * Find a vmap_area by address.
 *
 * @ptr		pointer into vmap_area
 **
 * Returns NULL or a pointer to a vmap_area.
 */
extern struct vmap_area *
find_vmap_area (void *ptr);

/**
 * Allocate a vmap_area and insert it into the vmap tree.
 *
 * @size	requested size of the area
 * @align	minimum area alignment
 * @low		lowest possible address (set this to KERNEL_VMAP_LOW)
 * @high	highest possible address (set this to KERNEL_VMAP_HIGH)
 *
 * Returns NULL or a pointer to a vmap_area.
 */
extern struct vmap_area *
allocate_vmap_area (const char *name,
	unsigned long size, unsigned long align,
	unsigned long low, unsigned long high);

/**
 * Remove a vmap_area from the vmap tree and free it.
 *
 * @area	vmap area to remove
 */
extern void
free_vmap_area (struct vmap_area *area);

/**
 * Map physical memory.
 *
 * @name	descriptive name of vmap_area
 * @addr	physical address to map
 * @size	size of memory to map
 * @prot	page protection flags (read/write and cache mode)
 * @align	required vmap_area alignment
 * @low		lowest possible address (set this to KERNEL_VMAP_LOW)
 * @high	highest possible address (set this to KERNEL_VMAP_HIGH)
 *
 * Returns NULL or a pointer to the mapped memory.
 */
extern void *
vmap_phys_explicit (const char *name,
	unsigned long addr, unsigned long size, pte_flags_t prot,
	unsigned long align, unsigned long low, unsigned long high);

/**
 * Map physical memory.
 *
 * @name	descriptive name of vmap_area
 * @addr	physical address to map
 * @size	size of memory to map
 * @prot	page protection flags (read/write and cache mode)
 *
 * Returns NULL or a pointer to the mapped memory.
 */
extern void *
vmap_phys (const char *name,
	unsigned long addr, unsigned long size, pte_flags_t prot);

/**
 * Map noncontiguous pages of memory into a virtually-contiguous space.
 *
 * @name	descriptive name of vmap_area
 * @pages	physical pages to map
 * @size	size of memory to map
 * @prot	page protection flags (read/write and cache mode)
 *
 * Returns NULL or a pointer to the mapped memory.
 */
extern void *
vmap_pages (const char *name,
	struct pfn_entry **pages, unsigned long nr_pages, pte_flags_t prot);

#define VMAP_PAGES_USES_GUARD_PAGES 1

/**
 * Unmap an area that was previously mapped using vmap.
 *
 * @mem		pointer returned from vmap_phys or vmap_pages
 */
extern void
vunmap (void *mem);

#endif /* _DAVIX_VMAP_H */
