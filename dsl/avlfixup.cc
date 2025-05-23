/**
 * AVLTree::fixup()
 * Copyright (C) 2025-present  dbstream
 */
#include <dsl/avltree.h>

namespace dsl {

static constexpr AVLTree::Dir LEFT = AVLTree::LEFT;
static constexpr AVLTree::Dir RIGHT = AVLTree::RIGHT;

static inline int
node_height (AVLNode *node)
{
	return node ? node->height : 0;
}

static inline int
new_height (AVLNode *node)
{
	int left = node_height (node->child[LEFT]);
	int right = node_height (node->child[RIGHT]);
	return 1 + (left > right ? left : right);
}

/**
 * node_balance - compute the node balance.
 * @node: node whose balance to compute
 *
 * This will be an integer in the range -2 to 2.  If this is -2, the tree is
 * left-heavy and must be rotated right.  If this is 2, the tree is right-heavy
 * and must be rotated left.
 */
static inline int
node_balance (AVLNode *node)
{
	return node_height (node->child[RIGHT]) - node_height (node->child[LEFT]);
}

/**
 * rotate - rotate a subtree.
 * @node: current root of the subtree
 * @dir: direction in which to rotate the subtree
 * Returns the new root of the subtree.
 */
static inline AVLNode *
rotate (AVLNode *node, AVLTree::Dir dir)
{
	AVLTree::Dir oth = (dir != LEFT) ? LEFT : RIGHT;
	AVLNode *Z = node->child[oth];
	AVLNode *tmp = Z->child[dir];

	if (tmp)
		tmp->parent = node;
	node->child[oth] = tmp;
	node->height = new_height (node);

	node->parent = Z;
	Z->child[dir] = node;
	Z->height = new_height (Z);

	return Z;
}

/**
 * AVLTree::fixup - fixup an AVL tree after insertion or deletion.
 * @node: node where the fixup should start
 */
void
AVLTree::fixup (AVLNode *node)
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
			 * We can exit early if our height is not updated,
			 * because then we know that no height change will
			 * propagate further upwards through the tree.
			 */
			int h = new_height (node);

			if (h == node->height)
				return;

			node->height = h;
			node = node->parent;
			continue;
		}

		AVLNode *parent = node->parent;
		AVLTree::Dir me = LEFT;
		if (parent && parent->child[LEFT] != node)
			me = RIGHT;

		/**
		 * Make dir the direction in which we are heavy, and oth the
		 * other direction.
		 */
		AVLTree::Dir dir = LEFT;
		AVLTree::Dir oth = RIGHT;
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
		AVLNode *Z = node->child[dir];
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
			m_root = node;

		node = parent;
	}
}

}
