/**
 * Keep track of resource ranges.
 * Copyright (C) 2025-present  dbstream
 *
 * Any device drivers should register themselves and their used IO and memory
 * ranges here.  This allows us to avoid conflicts between drivers wanting to
 * use the same memory range.
 *
 * Some drivers (e.g. fbcon) may want to support handover to other drivers (e.g.
 * a GPU driver).  This can be implemented based on ioresource.
 */
#include <davix/ioresource.h>
#include <davix/printk.h>
#include <davix/string.h>
#include <asm/ioresource.h>

struct ioresource iores_system_memory;
struct ioresource iores_system_io;

static void
iores_init (struct ioresource *iores, const char *name, 
		unsigned long start, unsigned long end)
{
	strncpy (iores->name, name, sizeof (iores->name));
	iores->name[sizeof (iores->name) - 1] = 0;
	iores->parent = NULL;
	spinlock_init (&iores->lock);
	iores->vma_node.first = start;
	iores->vma_node.last = end - 1;
	vma_tree_init (&iores->children);
}

void
init_ioresource (void)
{
	iores_init (&iores_system_memory, "SystemMemory",
			arch_iores_system_memory_start (),
			arch_iores_system_memory_end ());
	iores_init (&iores_system_io, "SystemIO",
			arch_iores_system_io_start (),
			arch_iores_system_io_end ());
}

void
dump_ioresource (struct ioresource *root)
{
	struct ioresource *parent = NULL, *current = root;
	bool ascending = false;
	int depth = 1;
	struct vma_tree_iterator it;

	spin_lock (&root->lock);
	do {
		if (ascending) {
			__spin_unlock (&current->lock);
			vma_tree_make_iterator (&it, &parent->children,
					&current->vma_node);
			if (!vma_tree_next (&it)) {
				depth--;
				current = parent;
				parent = current->parent;
				continue;
			}

			current = struct_parent (struct ioresource,
					vma_node, vma_node (&it));
			__spin_lock (&current->lock);
			ascending = false;
		}

		printk ("ioresource: %*s[0x%lx - 0x%lx]  %s\n", depth, "",
				iores_start (current), iores_end (current) - 1,
				current->name);

		if (!vma_tree_first (&it, &current->children)) {
			ascending = true;
			continue;
		}

		depth++;
		parent = current;
		current = struct_parent (struct ioresource,
				vma_node, vma_node (&it));
		__spin_lock (&current->lock);
	} while (current != root);
	spin_unlock (&root->lock);
}

errno_t
register_ioresource (struct ioresource *parent, struct ioresource *iores)
{
	struct vma_tree_iterator it;
	errno_t e = ESUCCESS;

	iores->name[sizeof (iores->name) - 1] = 0;
	iores->parent = parent;
	spinlock_init (&iores->lock);
	vma_tree_init (&iores->children);

	if (iores_start (iores) < iores_start (parent))
		return EINVAL;
	if (iores_end (iores) - 1UL > iores_end (parent) - 1UL)
		return EINVAL;

	spin_lock (&parent->lock);
	if (vma_tree_find_first (&it, &parent->children, iores_start (iores))) {
		if (vma_node (&it)->first < iores_end (iores))
			e = EEXIST;
	}

	if (e == ESUCCESS)
		vma_tree_insert (&parent->children, &iores->vma_node);
	spin_unlock (&parent->lock);
	return e;
}

void
unregister_ioresource (struct ioresource *iores)
{
	struct ioresource *parent = iores->parent;

	spin_lock (&parent->lock);
	vma_tree_remove (&parent->children, &iores->vma_node);
	spin_unlock (&parent->lock);
}

