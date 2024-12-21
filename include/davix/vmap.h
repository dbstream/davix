/**
 * Kernel virtual memory manager.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_VMAP_H
#define _DAVIX_VMAP_H 1

#include <davix/vma_tree.h>
#include <asm/page_defs.h>

struct vmap_area {
	struct vma_node vma_node;
	char purpose[32];
};

extern void
vmap_init (void);

extern void
vmap_dump (void);

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
 */
extern void *
vmap_phys (const char *name,
	unsigned long addr, unsigned long size, pte_flags_t prot);

#endif /* _DAVIX_VMAP_H */
