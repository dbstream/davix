/**
 * Operations on AVL trees
 * Copyright (C) 2024  dbstream
 *
 * An AVL tree is a family of self-balancing binary trees.
 *
 * This file defines operations on AVL trees.
 */
#ifndef _DAVIX_AVLTREE_H
#define _DAVIX_AVLTREE_H 1

#include <davix/avltree_types.h>

static inline void
avltree_init (struct avl_tree *tree)
{
	tree->root = 0;
}

static inline struct avl_node *
avl_left (struct avl_node *node)
{
	return node->child[0];
}

static inline struct avl_node *
avl_right (struct avl_node *node)
{
	return node->child[1];
}

static inline struct avl_node *
avl_parent (struct avl_node *node)
{
	return node->parent;
}

/**
 * Insert a node into the AVL tree at the specified location. parent must be
 * NULL, or parent->child[dir] must be NULL.
 */
extern void
__avl_insert (struct avl_tree *tree,
	struct avl_node *node, struct avl_node *parent, int dir);

/**
 * Remove a node from the AVL tree.
 */
extern void
__avl_remove (struct avl_tree *tree, struct avl_node *node);

/**
 * Insert a node into the AVL tree using the given comparator. The comparator
 * is a callable expression of the form cmp(node1, node2) that returns 1 if
 * node2 should be placed to the right of node1, 0 otherwise.
 */
#define avl_insert(tree, node, cmp) do {				\
	struct avl_tree *_avl_insert_tree = (tree);			\
	struct avl_node *_avl_insert_node = (node);			\
	struct avl_node *_avl_insert_parent = 0;			\
	struct avl_node *_avl_insert_curr = _avl_insert_tree->root;	\
	int _avl_insert_dir = 0;					\
	while (_avl_insert_curr) {					\
		_avl_insert_dir = cmp (_avl_insert_curr, _avl_insert_node); \
		_avl_insert_parent = _avl_insert_curr;			\
		_avl_insert_curr = _avl_insert_parent->child[_avl_insert_dir]; \
	}								\
	__avl_insert (_avl_insert_tree, _avl_insert_node,		\
		_avl_insert_parent, _avl_insert_dir);			\
} while (0)

#endif /* _DAVIX_AVLTREE_H */
