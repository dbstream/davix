/* SPDX-License-Identifier: MIT */
#include <davix/avltree.h>

static int height(struct avlnode *node)
{
	if(node)
		return node->height;
	return 0;
}

static int balance_factor(struct avlnode *node)
{
	return height(node->right) - height(node->left);
}

static void fixup_height(struct avlnode *node)
{
	node->height = max(height(node->left), height(node->right)) + 1;
}

/*
 * Do a left rotation.
 *
 * ``parent`` can have any value, as it is only used to update ``node->parent``
 * of the new sub-tree root node. ``link``, however, needs to be valid.
 *
 * NOTE: this function does NOT call ``avl_update_node()`` for any of the nodes.
 * ``avl_update_node()`` should be called for the old subtree root (``node``, or
 * the left child of the returned node) and the new subtree root
 * (``node->right``, or the returned node), AFTER this function is called.
 *
 * NOTE: this function also does NOT call ``fixup_height()`` for any of the
 * nodes.
 */
static struct avlnode *rotate_left(struct avlnode *node,
	struct avlnode *parent, struct avlnode **link)
{
	struct avlnode *right = node->right;
	struct avlnode *tmp = right->left;

	node->right = tmp;

	if(tmp)
		tmp->parent = node;

	right->left = node;
	node->parent = right;

	*link = right;
	right->parent = parent;

	return right;
}

/*
 * Do a right rotation.
 *
 * ``parent`` can have any value, as it is only used to update ``node->parent``
 * of the new sub-tree root node. ``link``, however, needs to be valid.
 *
 * NOTE: this function does NOT call ``avl_update_node()`` for any of the nodes.
 * ``avl_update_node()` should be called for the old subtree root (``node``, or
 * the right child of the returned node) and the new subtree root
 * (``node->left``, or the returned node), AFTER this function is called.
 *
 * NOTE: this function also does NOT call ``fixup_height()`` for any of the
 * nodes.
 */
static struct avlnode *rotate_right(struct avlnode *node,
	struct avlnode *parent, struct avlnode **link)
{
	struct avlnode *left = node->left;
	struct avlnode *tmp = left->right;

	node->left = tmp;

	if(tmp)
		tmp->parent = node;

	left->right = node;
	node->parent = left;

	*link = left;
	left->parent = parent;

	return left;
}

/*
 * Get the parent and link fields of a node in a tree.
 * If the node doesn't have a parent, returns ``&tree->root``.
 */
static void get_parent_and_link(struct avlnode *node, struct avltree *tree,
	struct avlnode **parent, struct avlnode ***link)
{
	struct avlnode *parent_ = node->parent;
	*parent = parent_;
	if(parent_) {
		*link = (node == parent_->left)
			? &parent_->left
			: &parent_->right;
	} else {
		*link = &tree->root;
	}
}

/*
 * Rebalance the node ``node`` and its parent nodes, calling
 * ``avl_update_node()`` when appropriate.
 */
static void fixup(struct avlnode *node, struct avltree *tree,
	avl_update_callback_t cb)
{
redo:;
	if(!node)
		return;

	struct avlnode *parent, **link;
	get_parent_and_link(node, tree, &parent, &link);

	int balance = balance_factor(node);
	if(balance > 1) {
		/* right-heavy */
		struct avlnode *newnode = node->right;

		if(height(newnode->left) > height(newnode->right)) {
			/* we need a double rotation */
			struct avlnode *old_newnode = newnode;
			newnode = rotate_right(newnode, node, &node->right);
			fixup_height(old_newnode);
			avl_update_node(old_newnode, cb);
		}

		rotate_left(node, parent, link);
		fixup_height(node);
		avl_update_node(node, cb);
		node = newnode;
	} else if(balance < -1) {
		/* left-heavy */
		struct avlnode *newnode = node->left;

		if(height(newnode->right) > height(newnode->left)) {
			/* we need a double rotation */
			struct avlnode *old_newnode = newnode;
			newnode = rotate_left(newnode, node, &node->left);
			fixup_height(old_newnode);
			avl_update_node(old_newnode, cb);
		}

		rotate_right(node, parent, link);
		fixup_height(node);
		avl_update_node(node, cb);
		node = newnode;
	}

	fixup_height(node);
	avl_update_node(node, cb);

	node = parent;
	goto redo;
}

void __avl_insert(struct avlnode *node, struct avlnode *parent,
	struct avlnode **link, struct avltree *tree,
	avl_update_callback_t callback)
{
	node->height = 1;
	node->left = NULL;
	node->right = NULL;
	node->parent = parent;
	*link = node;
	fixup(parent, tree, callback);
}

void __avl_delete(struct avlnode *node, struct avltree *tree,
	avl_update_callback_t callback)
{
	struct avlnode *parent, **link;
	get_parent_and_link(node, tree, &parent, &link);
	if(node->left && node->right) {
		/*
		 * Annoying case where we are deleting a node with two children.
		 *
		 * We find the in-order successor of the node, swap its location
		 * with the node we want to delete, and then fall-back to the
		 * simple delete case.
		 */
		struct avlnode *successor, *s_parent, **s_link;
		s_parent = node;
		s_link = &node->right;
		successor = node->right;
		while(successor->left) {
			s_parent = successor;
			s_link = &successor->left;
			successor = successor->left;
		}

		if(s_parent == node) {
			/*
			 * Special case where we did not go through a single
			 * iteration of the loop.
			 */
			successor->left = node->left;
			successor->left->parent = successor;
			node->left = NULL;

			node->right = successor->right;
			if(node->right)
				node->right->parent = node;

			successor->right = node;
			node->parent = successor;

			*link = successor;
			successor->parent = parent;

			swap(node->height, successor->height);
			parent = successor;
			link = &successor->right;
		} else {
			successor->left = node->left;
			successor->left->parent = successor;
			node->left = NULL;

			swap(node->right, successor->right);
			successor->right->parent = successor;
			if(node->right)
				node->right->parent = node;

			*s_link = node;
			node->parent = s_parent;

			*link = successor;
			successor->parent = parent;

			swap(node->height, successor->height);
			parent = s_parent;
			link = s_link;
		}
	}

	struct avlnode *child;
	child = node->left ?: node->right;
	*link = child;
	if(child)
		child->parent = parent;
	fixup(parent, tree, callback);
}
