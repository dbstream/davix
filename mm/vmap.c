/**
 * Kernel virtual memory manager.
 * Copyright (C) 2024  dbstream
 */
#include <davix/align.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/slab.h>
#include <davix/spinlock.h>
#include <davix/stddef.h>
#include <davix/string.h>
#include <davix/vmap.h>
#include <asm/mm_init.h>
#include <asm/page_defs.h>

static struct slab *vmap_slab;

static struct vma_tree vmap_tree;

/**
 * vmap_lock
 *
 * The vmap_lock must be held during any operation that reads or modifies the
 * vmap_tree.
 */
static spinlock_t vmap_lock;

static bool has_initialised_vmap = false;

static void
vmap_insert_default_area (unsigned long first, unsigned long last,
	const char *purpose)
{
	struct vmap_area *area = slab_alloc (vmap_slab);
	if (!area)
		panic ("vmap_init: failed to insert default area \"%s\"\n", purpose);

	area->vma_node.first = first;
	area->vma_node.last = last;
	strncpy (area->purpose, purpose, sizeof (area->purpose));
	area->purpose[sizeof (area->purpose) - 1] = 0;

	vma_tree_insert (&vmap_tree, &area->vma_node);
}

void
vmap_init (void)
{
	vma_tree_init (&vmap_tree);

	vmap_slab = slab_create ("vmap_area", sizeof (struct vmap_area));
	if (!vmap_slab)
		panic ("vmap_init: Failed to create the vmap slab!");

	has_initialised_vmap = true;

	arch_insert_vmap_areas (vmap_insert_default_area);
	vmap_dump ();
}

void
vmap_dump (void)
{
	if (!has_initialised_vmap)
		return;

	printk ("vmap_dump:\n");

	spin_lock (&vmap_lock);

	struct vma_tree_iterator it;
	bool flag = vma_tree_first (&it, &vmap_tree);
	while (flag) {
		struct vma_node *node = vma_node (&it);
		struct vmap_area *area = struct_parent (struct vmap_area, vma_node, node);
		printk ("... 0x%lx - 0x%lx    %s\n",
			area->vma_node.first, area->vma_node.last, area->purpose);

		flag = vma_tree_next (&it);
	}

	spin_unlock (&vmap_lock);
}

struct vmap_area *
allocate_vmap_area (const char *name,
	unsigned long size, unsigned long align,
	unsigned long low, unsigned long high)
{
	size = ALIGN_UP (size, PAGE_SIZE);
	align = max (unsigned long, align, PAGE_SIZE);
	if (!size || !align)
		return NULL;

	struct vmap_area *area = slab_alloc (vmap_slab);
	if (!area)
		return NULL;

	strncpy (area->purpose, name, sizeof (area->purpose));
	area->purpose[sizeof (area->purpose) - 1] = 0;

	spin_lock (&vmap_lock);

	unsigned long addr;
	if (vma_tree_find_free_bottomup (&addr, &vmap_tree, size, align, low, high)) {
		area->vma_node.first = addr;
		area->vma_node.last = addr + size - 1;
		vma_tree_insert (&vmap_tree, &area->vma_node);
	} else {
		slab_free (area);
		area = NULL;
	}

	spin_unlock (&vmap_lock);
	return area;
}
