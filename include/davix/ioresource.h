/**
 * Keep track of resource ranges.
 * Copyright (C) 2025-present  dbstream
 *
 * Any device drivers should register themselves and their used SystemIO and
 * SystemMemory ranges with ioresource.  This allows us to keep track of what
 * ranges are used, and thus avoid conflicts between drivers wanting to use
 * the same range.
 *
 * Some drivers (e.g. fbcon) may want to implement handover to dedicated device
 * drivers (such as a specific GPU driver).  This can be supported in the future
 * through an optional callback for device removal.
 *
 * In some cases (e.g. PCI BAR reallocation) we may want to allocate from an
 * ioresource.  Therefore use vma_tree.
 */
#ifndef _DAVIX_IORESOURCE_H
#define _DAVIX_IORESOURCE_H 1

#include <davix/errno.h>
#include <davix/vma_tree.h>
#include <davix/spinlock.h>

struct ioresource {
	spinlock_t lock;
	struct ioresource *parent;
	struct vma_node vma_node;
	struct vma_tree children;
	char name[40];
};

extern struct ioresource iores_system_memory;
extern struct ioresource iores_system_io;

static inline unsigned long
iores_start (struct ioresource *iores)
{
	return iores->vma_node.first;
}

static inline unsigned long
iores_end (struct ioresource *iores)
{
	return iores->vma_node.last + 1;
}

static inline unsigned long
iores_size (struct ioresource *iores)
{
	return iores_end (iores) - iores_start (iores);
}

extern void
init_ioresource (void);

extern errno_t
register_ioresource (struct ioresource *parent, struct ioresource *res);

extern void
unregister_ioresource (struct ioresource *res);

extern void
dump_ioresource (struct ioresource *root);

#endif /* _DAVIX_IORESOURCE_ */

