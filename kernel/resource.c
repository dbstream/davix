/* SPDX-License-Identifier: MIT */
#include <davix/resource.h>
#include <davix/avltree.h>
#include <davix/kmalloc.h>
#include <davix/printk.h>

static inline unsigned long subtree_gap(struct avlnode *node)
{
	if(!node)
		return 0;

	struct resource *resource = container_of(node, struct resource, avl);
	return resource->biggest_subtree_gap;
}

static void update_biggest_subtree_gap(struct avlnode *node)
{
	struct resource *resource = container_of(node, struct resource, avl);
	resource->biggest_subtree_gap = max(resource->end - resource->start,
		max(subtree_gap(node->left), subtree_gap(node->right)));
}

int init_resource(struct resource *resource,
	unsigned long start, unsigned long end,
	const char *name, struct resource *parent)
{
	struct resource *subresource = kmalloc(sizeof(struct resource));
	if(!subresource)
		return 1;

	resource->start = start;
	resource->end = end;
	resource->name = name;
	resource->children = AVLTREE_INIT(resource->children);
	resource->free = AVLTREE_INIT(resource->free);
	resource->parent = parent;
	spinlock_init(&resource->lock);

	subresource->start = start;
	subresource->end = end;
	subresource->biggest_subtree_gap = end - start;

	__avl_insert(&subresource->avl, NULL,
		&resource->free.root, &resource->free, NULL);

	return 0;
}

static void __dump_resource(struct resource *resource, int depth)
{
	int irqflag = spin_acquire(&resource->lock);
	info("resource: %*s[0x%lx - 0x%lx] %s\n", 8 * depth, "",
		resource->start, resource->end - 1, resource->name);

	for(struct avlnode *node = avl_first(&resource->children);
		node; node = avl_successor(node)) {
			struct resource *subresource =
				container_of(node, struct resource, avl);
			__dump_resource(subresource, depth + 1);
	}
	spin_release(&resource->lock, irqflag);
}

void dump_resource(struct resource *resource)
{
	__dump_resource(resource, 0);
}

static inline struct resource *avlnode_to_resource(struct avlnode *node)
{
	if(!node)
		return NULL;

	return container_of(node, struct resource, avl);
}

/*
 * Find a subresource just before a resource address.
 */
static struct resource *find_resource_before(struct avltree *tree,
	unsigned long addr)
{
	struct resource *resource = avlnode_to_resource(tree->root);
	struct resource *fallback = NULL;
	for(;;) {
		if(!resource)
			return fallback;

		if(resource->start == addr)
			return resource;

		if(resource->start > addr) {
			resource = avlnode_to_resource(resource->avl.left);
		} else {
			fallback = resource;
			resource = avlnode_to_resource(resource->avl.right);
		}
	}
}

static int avl_cmp_resources(struct avlnode *a, struct avlnode *b)
{
	struct resource *resource_a =
		container_of(a, struct resource, avl);

	struct resource *resource_b =
		container_of(b, struct resource, avl);

	return resource_b->start > resource_a->start ? 1 : -1;
}

static struct resource *__alloc_resource_at(struct resource *parent,
	unsigned long addr, unsigned long size, const char *name)
	must_hold(parent->lock)
{
	struct resource *from = find_resource_before(&parent->free, addr);
	if(!from)
		return NULL;

	unsigned long start = addr;
	unsigned long end = addr + size;

	if(from->start > start || from->end < end)
		return NULL;

	if(from->start == start) {
		if(from->end == end) {
			if(init_resource(from, start, end, name, parent))
					return NULL;

			__avl_delete(&from->avl, &parent->free,
				update_biggest_subtree_gap);

			avl_insert(&from->avl, &parent->children,
				avl_cmp_resources, NULL);

			return from;
		}

		struct resource *subresource = kmalloc(sizeof(struct resource));
		if(!subresource)
			return NULL;

		if(init_resource(subresource, start, end, name, parent)) {
				kfree(subresource);
				return NULL;
		}

		from->start = end;
		__avl_update_node_and_parents(&from->avl,
			update_biggest_subtree_gap);

		avl_insert(&subresource->avl, &parent->children,
			avl_cmp_resources, NULL);

		return subresource;
	}

	if(from->end == end) {
		struct resource *subresource = kmalloc(sizeof(struct resource));
		if(!subresource)
			return NULL;

		if(init_resource(subresource, start, end, name, parent)) {
				kfree(subresource);
				return NULL;
		}

		from->end = start;
		__avl_update_node_and_parents(&from->avl,
			update_biggest_subtree_gap);

		avl_insert(&subresource->avl, &parent->children,
			avl_cmp_resources, NULL);

		return subresource;
	}

	struct resource *other = kmalloc(sizeof(struct resource));
	if(!other)
		return NULL;

	struct resource *subresource = kmalloc(sizeof(struct resource));
	if(!subresource) {
		kfree(other);
		return NULL;
	}

	if(init_resource(subresource, start, end, name, parent)) {
			kfree(subresource);
			kfree(other);
			return NULL;
	}

	other->start = end;
	other->end = from->end;
	from->end = start;
	__avl_update_node_and_parents(&from->avl,
		update_biggest_subtree_gap);
	avl_insert(&other->avl, &parent->free,
		avl_cmp_resources, update_biggest_subtree_gap);

	avl_insert(&subresource->avl, &parent->children,
		avl_cmp_resources, NULL);

	return subresource;
}

struct resource *alloc_resource_at(struct resource *parent,
	unsigned long addr, unsigned long size, const char *name)
{
	int irqflag = spin_acquire(&parent->lock);
	struct resource *ret = __alloc_resource_at(parent, addr, size, name);
	spin_release(&parent->lock, irqflag);
	return ret;
}
