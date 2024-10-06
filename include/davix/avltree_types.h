/**
 * AVL tree type definition
 * Copyright (C) 2024  dbstream
 *
 * An AVL tree is a family of self-balancing binary trees.
 *
 * This file defines the structure of an AVL tree node.
 */
#ifndef _DAVIX_AVLTREE_TYPES_H
#define _DAVIX_AVLTREE_TYPES_H 1

/**
 * Please use helper functions avl_left, avl_right, and avl_parent instead of
 * directly using the fields in struct avl_node.
 */

struct avl_node {
	struct avl_node *child[2];
	struct avl_node *parent;
	int height;
};

struct avl_tree {
	struct avl_node *root;
};

#define AVLTREE_INIT { 0 }

#endif /* _DAVIX_AVLTREE_TYPES_H */
