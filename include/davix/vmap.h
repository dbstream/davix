/**
 * Kernel virtual memory manager.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_VMAP_H
#define _DAVIX_VMAP_H 1

#include <davix/vma_tree.h>

struct vmap_area {
	struct vma_node vma_node;
	char purpose[32];
};

extern void
vmap_init (void);

extern void
vmap_dump (void);

extern struct vmap_area *
allocate_vmap_area (const char *name,
	unsigned long size, unsigned long align,
	unsigned long low, unsigned long high);

#endif /* _DAVIX_VMAP_H */
