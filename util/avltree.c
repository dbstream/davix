/**
 * Operations on AVL trees
 * Copyright (C) 2024  dbstream
 */

#include <davix/avltree.h>
#include <davix/stddef.h>

static inline int
node_height (struct avl_node *node)
{
	return node ? node->height : 0;
}

static inline int
calc_new_height (struct avl_node *node)
{
	int left = node_height (node->child[0]);
	int right = node_height (node->child[1]);
	return 1 + max (int, left, right);
}

static inline int
node_balance (struct avl_node *node)
{
	return node_height (node->child[1]) - node_height (node->child[0]);
}

/* Rotate a subtree left. Returns the new root of the subtree. */
static struct avl_node *
rotate_left (struct avl_node *node)
{
	struct avl_node *Z = avl_right (node);
	struct avl_node *tmp = avl_left (Z);

	if (tmp)
		tmp->parent = node;
	node->child[1] = tmp;
	node->height = calc_new_height (node);

	node->parent = Z;
	Z->child[0] = node;
	Z->height = calc_new_height (Z);
	return Z;
}

/* Rotate a subtree right. Returns the new root of the subtree. */
static struct avl_node *
rotate_right (struct avl_node *node)
{
	struct avl_node *Z = avl_left (node);
	struct avl_node *tmp = avl_right (Z);

	if (tmp)
		tmp->parent = node;
	node->child[0] = tmp;
	node->height = calc_new_height (node);

	node->parent = Z;
	Z->child[1] = node;
	Z->height = calc_new_height (Z);
	return Z;
}

/* Fixup an AVL tree after insertion or deletion. */
static void
fixup (struct avl_tree *tree, struct avl_node *node)
{
	while (node) {
		node->height = calc_new_height (node);
		int balance = node_balance (node);

		if (balance >= 2) {
			struct avl_node *Z = avl_right (node);
			if (node_balance (Z) < 0) {
				Z = rotate_right (Z);
				Z->parent = node;
				node->child[1] = Z;
			}

			struct avl_node *Y = node;
			Z = node->parent;
			node = rotate_left (node);
			node->parent = Z;
			if (Z)
				Z->child[Z->child[1] == Y] = node;
			else
				tree->root = node;
		} else if (balance <= -2) {
			struct avl_node *Z = avl_left (node);
			if (node_balance (Z) > 0) {
				Z = rotate_left (Z);
				Z->parent = node;
				node->child[0] = Z;
			}

			struct avl_node *Y = node;
			Z = node->parent;
			node = rotate_right (node);
			node->parent = Z;
			if (Z)
				Z->child[Z->child[1] == Y] = node;
			else
				tree->root = node;
		} else
			node->height = calc_new_height (node);

		node = node->parent;
	}
}

void
__avl_insert (struct avl_tree *tree,
	struct avl_node *node, struct avl_node *parent, int dir)
{
	node->child[0] = NULL;
	node->child[1] = NULL;
	node->parent = parent;
	node->height = 1;

	if (!parent) {
		tree->root = node;
		return;
	}

	parent->child[dir] = node;
	fixup (tree, parent);
}

void
__avl_remove (struct avl_tree *tree, struct avl_node *node)
{
	struct avl_node *parent = node->parent;
	if (!node->child[0] || !node->child[1]) {
		struct avl_node *Z = node->child[node->child[0] == NULL];
		if (Z)
			Z->parent = parent;

		if (parent)
			parent->child[parent->child[1] == node] = Z;
		else
			tree->root = Z;

		fixup (tree, parent);
		return;
	}

	struct avl_node *Z = avl_right (node);
	if (!avl_left (Z)) {
		Z->parent = parent;
		Z->child[0] = avl_left (node);
		Z->child[0]->parent = Z;
		if (parent)
			parent->child[parent->child[1] == node] = Z;
		else
			tree->root = Z;
		fixup (tree, Z);
		return;
	}

	do
		Z = avl_left (Z);
	while (avl_left (Z));

	struct avl_node *Y = Z->parent;
	Y->child[0] = NULL;

	Z->parent = parent;
	if (parent)
		parent->child[parent->child[1] == node] = Z;
	else
		tree->root = Z;

	Z->child[0] = avl_left (node);
	Z->child[0]->parent = Z;
	Z->child[1] = avl_right (node);
	Z->child[1]->parent = Z;
	fixup (tree, Y);
}
