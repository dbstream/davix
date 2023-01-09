/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_AVLTREE_H
#define __DAVIX_AVLTREE_H

#include <davix/types.h>

/*
 * AVL tree implementation.
 */

#define AVLTREE_INIT(name) (struct avltree) { NULL };

/*
 * AVL tree child update callback.
 *
 * This function (if non-NULL) is called whenever the child pointers of a node
 * have changed. This function is also called for parents of all nodes that it
 * is called for, all the way up to the root of the tree.
 *
 * Why might this be useful?
 *
 * Consider someone implementing a virtual memory area allocator. It'd be useful
 * to know very fast if there is a big enough area in either sub-tree of a node.
 *
 * Using this callback, implementing that becomes very simple. Simply keep
 * track of an additional variable in the structure embedding avlnode,
 * ``biggest_gap``, which is the maximum of the gap of the node itself and its
 * children.
 */
typedef void (*avl_update_callback_t)(struct avlnode *a);

static inline void avl_update_node(struct avlnode *node,
	avl_update_callback_t callback)
{
	if(callback)
		callback(node);
}

/*
 * Insert a node into the tree at the specified location.
 *
 * Arguments:
 * - ``node``: The node to insert.
 * - ``parent``: The soon-to-be parent of the inserted node, or, if the tree
 *               is empty, NULL.
 * - ``link``: The pointer to be overwritten with ``node``.
 * - ``tree``: The AVL tree to insert into.
 * - ``callback``: The avl_update_callback_t to be used.
 */
void __avl_insert(struct avlnode *node, struct avlnode *parent,
	struct avlnode **link, struct avltree *tree,
	avl_update_callback_t callback);

/*
 * Delete a node from the tree.
 *
 * Arguments:
 * - ``node``: The node to delete.
 * - ``tree``: The AVL tree containing ``node``.
 * - ``callback``: The avl_update_callback_t to be used.
 */
void __avl_delete(struct avlnode *node, struct avltree *tree,
	avl_update_callback_t callback);

/*
 * Call a function for the node and every parent node.
 */
static inline void __avl_update_node_and_parents(struct avlnode *node,
	avl_update_callback_t callback)
{
	if(!callback)
		return;

	for(; node; node = node->parent)
		callback(node);
}

/*
 * Insert a node into the tree using a comparison function.
 *
 * Arguments:
 * - ``node``: The node to insert.
 * - ``tree``: The tree to insert the node into.
 * - ``cmp``: Comparison function.
 *            If ``b > a``, return a positive integer.
 *            If ``b < a``, return a negative integer.
 */
static inline void avl_insert(struct avlnode *node, struct avltree *tree,
	int (*cmp)(struct avlnode *a, struct avlnode *b),
	avl_update_callback_t callback)
{
	struct avlnode *searchnode, *parent, **link;

	parent = NULL;
	link = &tree->root;
	searchnode = tree->root;

	while(searchnode) {
		parent = searchnode;
		link = (cmp(searchnode, node) > 0)
			? &searchnode->right
			: &searchnode->left;
		searchnode = *link;
	}

	__avl_insert(node, parent, link, tree, callback);
}

/*
 * Delete a node from the tree using a comparison function.
 *
 * Arguments:
 * - ``key``: A key to find the node.
 * - ``tree``: The tree to delete the node from.
 * - ``cmp``: Comparison function.
 *            If ``key > node``, return a positive integer.
 *            If ``key < node``, return a negative integer.
 *            If ``key == node``, return zero.
 *
 * Return value: The deleted node.
 */
static inline struct avlnode *avl_delete(void *key, struct avltree *tree,
	int (*cmp)(struct avlnode *node, void *key),
	avl_update_callback_t callback)
{
	struct avlnode *node = tree->root;

	while(1) {
		int diff = cmp(node, key);
		if(diff == 0) {
			__avl_delete(node, tree, callback);
			return node;
		} else {
			node = (diff > 0) ? node->right : node->left;
		}
	}
}

static inline struct avlnode *avl_first(struct avltree *tree)
{
	struct avlnode *node = tree->root;
	if(!node)
		return NULL;

	while(node->left)
		node = node->left;

	return node;
}

static inline struct avlnode *avl_last(struct avltree *tree)
{
	struct avlnode *node = tree->root;
	if(!node)
		return NULL;

	while(node->right)
		node = node->right;

	return node;
}

static inline struct avlnode *avl_predecessor(struct avlnode *node)
{
	if(node->left) {
		node = node->left;
		while(node->right)
			node = node->right;
		return node;
	}

	while(node->parent) {
		if(node->parent->left == node)
			return node->parent;
		node = node->parent;
	}

	return NULL;
}

static inline struct avlnode *avl_successor(struct avlnode *node)
{
	if(node->right) {
		node = node->right;
		while(node->left)
			node = node->left;
		return node;
	}

	while(node->parent) {
		if(node->parent->left == node)
			return node->parent;
		node = node->parent;
	}

	return NULL;
}

#endif /* __DAVIX_AVLTREE_H */
