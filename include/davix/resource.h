/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_RESOURCE_H
#define __DAVIX_RESOURCE_H

#include <davix/types.h>
#include <davix/spinlock.h>

/*
 * A generic interface for managing system resources like physical address
 * space.
 */

struct resource {
	/*
	 * The range this resource covers.
	 * start..end, inclusive of start but not of end.
	 */
	unsigned long start;
	unsigned long end;

	/*.
	 * For "free" nodes, the biggest free gap in this AVL subtree.
	 */
	unsigned long biggest_subtree_gap;

	/*
	 * Parent resource or NULL, and AVL structure for keeping track of
	 * multiple resources.
	 */
	struct resource *parent;
	struct avlnode avl;

	/*
	 * Child resources of this resource.
	 */
	struct avltree children;

	/*
	 * Free (unused) resource space.
	 *
	 * This is a tree of ``struct resource``s for which some fields are
	 * invalid. For example, the ``parent``, ``children`` and ``free``
	 * fields within ``struct resource``s in this tree are not valid.
	 */
	struct avltree free;

	/*
	 * Protect alloc_resource(), free_resource() interfaces against
	 * concurrent usage.
	 */
	spinlock_t lock;

	/*
	 * A human-readable name for this resource.
	 */
	union {
		const char *name;
		char *__name;
	};
};

/*
 * Initializes all fields of a ``struct resource``.
 * Returns non-zero on failure.
 */
int init_resource(struct resource *resource,
	unsigned long start, unsigned long end,
	const char *name, struct resource *parent);

void dump_resource(struct resource *resource);

/*
 * Allocate a resource from a parent resource at a specific address.
 * Return value: subresource pointer if successful, otherwise NULL.
 */
struct resource *alloc_resource_at(struct resource *parent,
	unsigned long addr, unsigned long size, const char *name);

#endif /* __DAVIX_RESOURCE_H */
