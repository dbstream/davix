/**
 * VMA tree datastructure.
 * Copyright (C) 2024  dbstream
 *
 * The VMA tree is an augmented AVL tree that tracks the biggest gap between
 * two entries in a subtree, to support fast lookup of free ranges of a given
 * size. Additionally, the VMA tree keeps a linked list of entries.
 *
 * Users of the VMA tree API must call vma_tree_adjust() if it updates the first
 * or last values of any node.
 */
#ifndef _DAVIX_VMA_TREE_H
#define _DAVIX_VMA_TREE_H 1

#include <davix/list.h>

struct vma_tree {
	struct list vma_list;
	struct vma_node *vma_tree;
};

static inline void
vma_tree_init (struct vma_tree *tree)
{
	list_init (&tree->vma_list);
	tree->vma_tree = NULL;
}

struct vma_node {
	/** Linked list of nodes.  */
	struct list vma_list;

	/** The range that this node covers, inclusive.  */
	unsigned long first, last;

	/** standard AVL tree linkage  */
	struct vma_node *children[2];
	struct vma_node *parent;
	int height;

	/** Keep track of the gap to the previous entry. Propagate upwards.  */
	unsigned long biggest_gap;
	unsigned long prev_gap;
};

struct vma_tree_iterator {
	struct vma_tree *tree;
	struct list *entry;
};

static inline void
vma_tree_make_iterator (struct vma_tree_iterator *it,
	struct vma_tree *tree, struct vma_node *entry)
{
	it->tree = tree;
	it->entry = &entry->vma_list;
}

static inline bool
vma_tree_first (struct vma_tree_iterator *it, struct vma_tree *tree)
{
	it->tree = tree;
	it->entry = tree->vma_list.next;
	return it->entry != &tree->vma_list;
}

static inline bool
vma_tree_last (struct vma_tree_iterator *it, struct vma_tree *tree)
{
	it->tree = tree;
	it->entry = tree->vma_list.prev;
	return it->entry != &tree->vma_list;
}

static inline bool
vma_tree_next (struct vma_tree_iterator *it)
{
	it->entry = it->entry->next;
	return it->entry != &it->tree->vma_list;
}

static inline bool
vma_tree_prev (struct vma_tree_iterator *it)
{
	it->entry = it->entry->prev;
	return it->entry != &it->tree->vma_list;
}

static inline struct vma_node *
vma_node (struct vma_tree_iterator *it)
{
	return list_item (struct vma_node, vma_list, it->entry);
}

/**
 * Find @addr in @tree. Stores the matching node (if any) in @it. Returns false
 * if no node contains @addr.
 */
extern bool
vma_tree_find (struct vma_tree_iterator *it,
	struct vma_tree *tree, unsigned long addr);

/**
 * Insert @node into @tree.
 */
extern void
vma_tree_insert (struct vma_tree *tree, struct vma_node *node);

/**
 * Remove @node from @tree.
 */
extern void
vma_tree_remove (struct vma_tree *tree, struct vma_node *node);

/**
 * Update auxiliary data in @tree after having edited @node.
 */
extern void
vma_tree_adjust (struct vma_tree *tree, struct vma_node *node);

/**
 * Search @tree bottom-up for a free range of size @size, alignment @align,
 * no lower than @min_addr and no higher than @max_addr. If a free range is
 * found, it is returned in @out and the function returns true. Otherwise,
 * the function returns false.
 */
extern bool
vma_tree_find_free_bottomup (unsigned long *out, struct vma_tree *tree,
	unsigned long size, unsigned long align,
	unsigned long min_addr, unsigned long max_addr);

/**
 * Search @tree top-down for a free range of size @size, alignment @align,
 * no lower than @min_addr and no higher than @max_addr. If a free range is
 * found, it is returned in @out and the function returns true. Otherwise,
 * the function returns false.
 */
extern bool
vma_tree_find_free_topdown (unsigned long *out, struct vma_tree *tree,
	unsigned long size, unsigned long align,
	unsigned long min_addr, unsigned long max_addr);

#endif /* _DAVIX_VMA_TREE_H */
