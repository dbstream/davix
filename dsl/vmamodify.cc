/**
 * Modification operations on VMA trees.
 * Copyright (C) 2025-present  dbstream
 */
#include <dsl/minmax.h>
#include <dsl/vmatree.h>

namespace dsl {

static constexpr VMATree::Dir LEFT = VMATree::LEFT;
static constexpr VMATree::Dir RIGHT = VMATree::RIGHT;

static inline int
node_height (VMANode *node)
{
	return node ? node->height : 0;
}

static inline int
node_balance (VMANode *node)
{
	return node_height (node->child[RIGHT]) - node_height (node->child[LEFT]);
}

static inline uintptr_t
node_max_gap (VMANode *node)
{
	return node ? node->biggest_gap : 0;
}

/** Propagate auxiliary data upwards.  Returns true if anything changed.  */
static inline bool
propagate (VMANode *node)
{
	int l_height = node_height (node->child[LEFT]);
	uintptr_t l_gap = node_max_gap (node->child[LEFT]);
	int r_height = node_height (node->child[RIGHT]);
	uintptr_t r_gap = node_max_gap (node->child[RIGHT]);

	int new_height = 1 + max (l_height, r_height);
	uintptr_t new_gap = max (node->prev_gap, max (l_gap, r_gap));

	if (new_height == node->height && new_gap == node->biggest_gap)
		return false;

	node->height = new_height;
	node->biggest_gap = new_gap;
	return true;
}

/**
 * rotate - rotate a subtree.
 * @node: current root of subtree
 * @dir: direction in which to rotate the subtree
 * Returns the new root of the subtree.
 */
static inline VMANode *
rotate (VMANode *node, VMATree::Dir dir)
{
	VMATree::Dir oth = (dir != LEFT) ? LEFT : RIGHT;
	VMANode *Z = node->child[oth];
	VMANode *tmp = Z->child[dir];

	node->child[oth] = tmp;
	if (tmp)
		tmp->parent = node;

	Z->child[dir] = node;
	node->parent = Z;
	propagate (node);
	propagate (Z);
	return Z;
}

/**
 * VMATree::fixup - fixup the tree after modification.
 * @node: node to start the fixup at
 *
 * This is mostly the same as AVLTree::fixup except different propagation
 * conditions.
 */
void
VMATree::fixup (VMANode *node)
{
	while (node) {
		int balance = node_balance (node);

		/**
		 * Test if we are already balanced.
		 */
		if (-1 <= balance && balance <= 1) {
			/**
			 * We are balanced: check if we can exit the loop early.
			 *
			 * We can exit the loop early if propagation does not
			 * cause any changes.
			 */
			if (!propagate (node))
				return;

			node = node->parent;
			continue;
		}

		VMANode *parent = node->parent;
		VMATree::Dir me = LEFT;
		if (parent && parent->child[LEFT] != node)
			me = RIGHT;

		/**
		 * Make dir the direction in which we are heavy, and oth the
		 * other direction.
		 */
		VMATree::Dir dir = LEFT;
		VMATree::Dir oth = RIGHT;
		if (balance >= 0) {
			dir = RIGHT;
			oth = LEFT;
		}


		/**
		 * The following situation may arise:
		 *       A               B
		 *        \             /
		 *         B    <->    A
		 *        /             \
		 *       C               C
		 * , in which case a left rotation on A or a right rotation on B
		 * would result in a tree which is still imbalanced.
		 *
		 * Avoid this by detecting the situation and rotating the
		 * relevant child in the opposite direction.
		 */
		VMANode *Z = node->child[dir];
		if (dir == LEFT && node_balance (Z) > 0) {
			Z = rotate (Z, LEFT);
			Z->parent = node;
			node->child[LEFT] = Z;
		} else if (dir == RIGHT && node_balance (Z) < 0) {
			Z = rotate (Z, RIGHT);
			Z->parent = node;
			node->child[RIGHT] = Z;
		}

		Z = node;
		node = rotate (node, oth);
		node->parent = parent;
		if (parent)
			parent->child[me] = node;
		else
			root = node;

		node = parent;
	}
}

/* Insert a node into the tree.  */
void
VMATree::insert (VMANode *node)
{
	node->child[LEFT] = nullptr;
	node->child[RIGHT] = nullptr;
	node->height = 1;

	VMANode *parent = nullptr;
	VMANode *x = root;
	Dir dir = LEFT;
	while (x) {
		if (node->first < x->first)
			dir = LEFT;
		else
			dir = RIGHT;

		parent = x;
		x = x->child[dir];
	}

	VMANode *pred, *succ;
	if (dir == RIGHT) {
		pred = parent;
		succ = next (parent);
	} else {
		pred = prev (parent);
		succ = parent;
	}

	if (pred)
		node->prev_gap = node->first - pred->last - 1;
	else
		node->prev_gap = node->first;

	if (succ)
		succ->prev_gap = succ->first - node->last - 1;

	node->parent = parent;
	if (parent)
		parent->child[dir] = node;
	else
		root = node;

	if (pred)
		pred->list.push_front (&node->list);
	else
		list.push_front (node);

	fixup (node);
}

/* Remove a node from the tree.  */
void
VMATree::remove (VMANode *node)
{
	VMANode *pred, *succ, *Z, *parent = node->parent;

	Dir me = LEFT;
	if (parent && parent->child[LEFT] != node)
		me = RIGHT;

	/** Node deletion changes prev_gap of the successor.  */

	pred = prev (node);
	succ = next (node);

	if (succ) {
		if (pred)
			succ->prev_gap = succ->first - pred->last - 1;
		else
			succ->prev_gap = succ->first;
	}

	node->list.remove ();

	if (!node->child[RIGHT]) {
		/**
		 * Our right child is nullptr and thus the successor is in the
		 * fixup chain.
		 */
		Z = node->child[LEFT];
		if (parent)
			parent->child[me] = Z;
		else
			root = Z;

		if (Z)
			Z->parent = parent;
		else
			Z = parent;
		fixup (Z);
		return;
	}

	if (!node->child[LEFT]) {
		/**
		 * The successor node is somewhere in the right subtree.
		 */
		Z = node->child[RIGHT];
		Z->parent = parent;
		if (parent)
			parent->child[me] = Z;
		else
			root = Z;

		fixup (succ);
		return;
	}

	/** Argh, this is the annoying case.  Replace node with succ.  */

	Z = succ;
	VMANode *Y = Z->parent;
	Z->parent = parent;
	Z->child[LEFT] = node->child[LEFT];
	Z->child[LEFT]->parent = Z;
	if (parent)
		parent->child[me] = Z;
	else
		root = Z;

	if (Y == node) {
		/** The successor is the direct child of node.  */
		fixup (Z);
		return;
	}

	/* We need to be careful regarding the right subtree.  */
	Y->child[LEFT] = Z->child[RIGHT];
	if (Y->child[LEFT])
		Y->child[LEFT]->parent = Y;
	Z->child[RIGHT] = node->child[RIGHT];
	Z->child[RIGHT]->parent = Z;

	fixup (Y);
}

/* Update tree auxiliary data after having modified node.  */
void
VMATree::adjust (VMANode *node)
{
	VMANode *pred = prev (node), *succ = next (node);

	if (pred)
		node->prev_gap = node->first - pred->last - 1;
	else
		node->prev_gap = node->first;

	if (succ) {
		succ->prev_gap = succ->first - node->last - 1;
		if (succ->height < node->height)
			node = succ;
	}

	do {
		/**
		 * We could do early exiting here.  But that becomes annoying
		 * because we have to make sure that we touch both the node and
		 * its successor.
		 */
		propagate (node);
		node = node->parent;
	} while (node);
}

}
