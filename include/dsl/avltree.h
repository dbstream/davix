/**
 * AVL trees.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

namespace dsl {

struct AVLNode {
	AVLNode *child[2];
	AVLNode *parent;
	int height;
};

class AVLTree {
public:
	enum Dir { LEFT = 0, RIGHT = 1 };

private:
	AVLNode *m_root = nullptr;

	void
	fixup (AVLNode *fixup_node);

public:
	/**
	 * AVLTree::init - initialize the AVL tree.
	 */
	constexpr inline void
	init (void)
	{
		m_root = nullptr;
	}

	/**
	 * AVLTree::empty - test if an AVL tree is empty.
	 */
	constexpr bool
	empty (void) const
	{
		return m_root == nullptr;
	}

	/**
	 * AVLTree::insert_at - insert a node into the tree.
	 * @parent: parent node
	 * @dir: which child to become
	 * @node: the node to insert
	 */
	inline void
	insert_at (AVLNode *parent, Dir dir, AVLNode *node)
	{
		node->child[LEFT] = nullptr;
		node->child[RIGHT] = nullptr;
		node->parent = parent;
		node->height = 1;

		if (!parent) {
			m_root = node;
			return;
		}

		parent->child[dir] = node;
		fixup (parent);
	}

	/**
	 * AVLTree::remove - remove a node from the tree.
	 * @node: which node to remove
	 */
	inline void
	remove (AVLNode *node)
	{
		AVLNode *parent = node->parent;

		/**
		 * Set 'me' to the direction in which we are from our parent.
		 */
		Dir me = LEFT;
		if (parent && parent->child[LEFT] != node)
			me = RIGHT;

		/**
		 * Handle the easy case: at least one of our children is NULL.
		 */
		if (!node->child[LEFT] || !node->child[1]) {
			AVLNode *Z = node->child[LEFT];
			if (!Z)
				Z = node->child[RIGHT];

			if (Z)
				Z->parent = parent;

			if (parent) {
				parent->child[me] = Z;
				fixup (parent);
			} else
				m_root = Z;
			return;
		}

		/**
		 * None of our children are NULL.  Find the in-order successor.
		 */
		AVLNode *Z = node->child[RIGHT];
		if (!Z->child[LEFT]) {
			/**
			 * Special case: there is no node between us and our
			 * in-order successor.
			 */
			Z->parent = parent;
			Z->child[LEFT] = node->child[LEFT];
			Z->child[LEFT]->parent = Z;

			/** NB: need fixup in either case  */
			if (parent)
				parent->child[me] = Z;
			else
				m_root = Z;
			fixup (Z);
			return;
		}

		do {
			Z = Z->child[LEFT];
		} while (Z->child[LEFT] != nullptr);

		AVLNode *Y = Z->parent;
		Y->child[LEFT] = Z->child[RIGHT];
		if (Y->child[LEFT])
			Y->child[LEFT]->parent = Y;

		Z->parent = parent;
		if (parent)
			parent->child[me] = Z;
		else
			m_root = Z;

		Z->child[LEFT] = node->child[LEFT];
		Z->child[LEFT]->parent = Z;
		Z->child[RIGHT] = node->child[RIGHT];
		Z->child[RIGHT]->parent = Z;
		fixup (Y);
	}
};

}
