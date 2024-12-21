/**
 * VMA tree datastructure.
 * Copyright (C) 2024  dbstream
 *
 * The VMA tree is an augmented AVL tree that tracks the biggest gap between
 * two entries in a subtree, to support fast lookup of free ranges of a given
 * size. Additionally, the VMA tree keeps a linked list of entries.
 */
#include <davix/align.h>
#include <davix/vma_tree.h>

bool
vma_tree_find (struct vma_tree_iterator *it,
	struct vma_tree *tree, unsigned long addr)
{
	it->tree = tree;

	struct vma_node *current = tree->vma_tree;
	while (current) {
		if (addr < current->first) {
			current = current->children[0];
			continue;
		}

		if (addr > current->last) {
			current = current->children[1];
			continue;
		}

		it->entry = &current->vma_list;
		return true;
	}

	it->entry = NULL;
	return false;
}

static inline struct vma_node *
prev_node (struct vma_tree *tree, struct vma_node *node)
{
	if (node->vma_list.prev == &tree->vma_list)
		return NULL;
	return list_item (struct vma_node, vma_list, node->vma_list.prev);
}

static inline struct vma_node *
next_node (struct vma_tree *tree, struct vma_node *node)
{
	if (node->vma_list.next == &tree->vma_list)
		return NULL;
	return list_item (struct vma_node, vma_list, node->vma_list.next);
}

bool
vma_tree_find_free_bottomup (unsigned long *out, struct vma_tree *tree,
	unsigned long size, unsigned long align,
	unsigned long min_addr, unsigned long max_addr)
{
	struct vma_node *tmp, *current = tree->vma_tree;
	unsigned long alloc_start, gap_start, gap_end;

	if (!size)
		return false;

	if (max_addr < min_addr || max_addr < size)
		return false;

	if ((max_addr != -1UL || min_addr != 0UL) && max_addr - min_addr + 1 < size)
		return false;

	if (!current)
		goto look_at_rightmost;
	for (;;) {
		/** If no node in the current subtree is big enough, skip it. */
		if (current->biggest_gap < size)
			goto go_up;

		/** Do not look at the left subtree if it is below min_addr.  */
		if (current->children[0] && current->first > min_addr) {
			current = current->children[0];
			continue;
		}

look_at_current:
		tmp = prev_node (tree, current);
		if (tmp)
			gap_start = tmp->last + 1;
		else
			gap_start = 0;
		gap_end = current->first;

		gap_start = max (unsigned long, gap_start, min_addr);
		if (max_addr != -1UL)
			gap_end = min (unsigned long, gap_end, max_addr + 1);

		alloc_start = ALIGN_UP (gap_start, align);
		if (alloc_start < gap_start || gap_end <= alloc_start)
			goto look_at_right_subtree;

		if (gap_end - alloc_start >= size) {
			*out = alloc_start;
			return true;
		}

		/** Do not keep looking for free areas above max_addr.  */
		if (gap_end >= max_addr)
			return false;

look_at_right_subtree:
		if (current->children[1]) {
			current = current->children[1];
			continue;
		}

go_up:		tmp = current;
		current = current->parent;
		if (!current)
			break;

		if (current->children[1] == tmp)
			goto go_up;

		goto look_at_current;
	}

	/** Look at the gap after the rightmost node in the tree.  */
look_at_rightmost:
	if (tree->vma_list.prev != &tree->vma_list) {
		current = list_item (struct vma_node, vma_list, tree->vma_list.prev);
		if (current->last == -1UL)
			return false;
		gap_start = current->last + 1;
	} else
		gap_start = 0;

	gap_start = max (unsigned long, gap_start, min_addr);
	alloc_start = ALIGN_UP (gap_start, align);
	if (alloc_start < gap_start)
		return false;

	gap_end = alloc_start + size - 1;
	if (gap_end < alloc_start || gap_end > max_addr)
		return false;

	*out = alloc_start;
	return true;
}

bool
vma_tree_find_free_topdown (unsigned long *out, struct vma_tree *tree,
	unsigned long size, unsigned long align,
	unsigned long min_addr, unsigned long max_addr)
{
	struct vma_node *tmp, *current = tree->vma_tree;
	unsigned long alloc_start, gap_start, gap_end;

	if (!size)
		return false;

	if (max_addr < min_addr || max_addr < size)
		return false;

	if ((max_addr != -1UL || min_addr != 0UL) && max_addr - min_addr + 1 < size)
		return false;

	if (current) {
		tmp = list_item (struct vma_node, vma_list, tree->vma_list.prev);
		if (tmp->last == -1UL)
			goto start_loop;
		gap_start = tmp->last + 1;
	} else
		gap_start = 0;

	gap_start = max (unsigned long, gap_start, min_addr);
	alloc_start = ALIGN_UP (gap_start, align);
	if (alloc_start < gap_start)
		goto start_loop;

	gap_end = alloc_start + size - 1;
	if (gap_end >= alloc_start && gap_end <= max_addr) {
		*out = ALIGN_DOWN (max_addr + 1 - size, align);
		return true;
	}

	if (!current)
		return false;

start_loop:
	for (;;) {
		/** If no node in the current subtree is big enough, skip it. */
		if (current->biggest_gap < size)
			goto go_up;

		/** Do not look at the right subtree if it is above max_addr. */
		if (current->children[1] && current->last < max_addr) {
			current = current->children[1];
			continue;
		}

look_at_current:
		tmp = prev_node (tree, current);
		if (tmp)
			gap_start = tmp->last + 1;
		else
			gap_start = 0;
		gap_end = current->first;

		gap_start = max (unsigned long, gap_start, min_addr);
		if (max_addr != -1UL)
			gap_end = min (unsigned long, gap_end, max_addr + 1);

		alloc_start = ALIGN_UP (gap_start, align);
		if (alloc_start < gap_start || gap_end <= alloc_start)
			goto look_at_left_subtree;

		if (gap_end - alloc_start >= size) {
			*out = ALIGN_DOWN (gap_end - size, align);
			return true;
		}

		/** Do not keep looking for free areas below min_addr.  */
		if (gap_start <= min_addr)
			return false;

look_at_left_subtree:
		if (current->children[0]) {
			current = current->children[0];
			continue;
		}

go_up:		tmp = current;
		current = current->parent;
		if (!current)
			return false;

		if (current->children[0] == tmp)
			goto go_up;

		goto look_at_current;
	}
}

static inline int
node_height (struct vma_node *node)
{
	if (node)
		return node->height;
	return 0;
}

static int
node_balance (struct vma_node *node)
{
	return node_height (node->children[1]) - node_height (node->children[0]);
}

static inline unsigned long
node_max_gap (struct vma_node *node)
{
	if (node)
		return node->biggest_gap;
	return 0;
}

/**
 * Propagate height and biggest_gap from the children of @node.
 */
static inline void
propagate_height_and_gap (struct vma_node *node)
{
	int left_height = node_height (node->children[0]);
	int right_height = node_height (node->children[1]);
	unsigned long left_gap = node_max_gap (node->children[0]);
	unsigned long right_gap = node_max_gap (node->children[1]);

	node->height = 1 + max (int, left_height, right_height);
	node->biggest_gap = max (unsigned long, node->prev_gap,
		max (unsigned long, left_gap, right_gap));
}

/**
 * Rotate the subtree rooted at @node left. Returns the new root.
 *
 * Visualization of operation:
 *     before                         after
 *      node                            Z
 *           Z                     node
 *      tmp                            tmp
 */
static struct vma_node *
rotate_left (struct vma_node *node)
{
	struct vma_node *Z = node->children[1];
	struct vma_node *tmp = Z->children[0];

	node->children[1] = tmp;
	if (tmp)
		tmp->parent = node;

	Z->children[0] = node;
	node->parent = Z;

	propagate_height_and_gap (node);
	propagate_height_and_gap (Z);
	return Z;
}

/**
 * Rotate the subtree rooted at @node right. Returns the new root.
 *
 * Visualization of operation:
 *     before                         after
 *      node                            Z
 *    Z                                   node
 *      tmp                            tmp
 */
static struct vma_node *
rotate_right (struct vma_node *node)
{
	struct vma_node *Z = node->children[0];
	struct vma_node *tmp = Z->children[1];

	node->children[0] = tmp;
	if (tmp)
		tmp->parent = node;

	Z->children[1] = node;
	node->parent = Z;

	propagate_height_and_gap (node);
	propagate_height_and_gap (Z);
	return Z;
}

/**
 * Iteratively fixup the tree, starting at @node.
 */
static void
fixup_tree (struct vma_tree *tree, struct vma_node *node)
{
	while (node) {
		int balance = node_balance (node);

		if (balance >= 2) {
			/** The subtree is right-heavy.  Rotate left  */

			struct vma_node *Y = node, *Z = node->children[1];
			if (node_balance (Z) < 0) {
				Z = rotate_right (Z);
				Z->parent = node;
				node->children[1] = Z;
			}

			Z = node->parent;
			node = rotate_left (node);
			node->parent = Z;

			if (Z)
				Z->children[Z->children[1] == Y] = node;
			else
				tree->vma_tree = node;

		} else if (balance <= -2) {
			/** The subtree is left-heavy.  Rotate right  */

			struct vma_node *Y = node, *Z = node->children[0];
			if (node_balance (Z) > 0) {
				Z = rotate_left (Z);
				Z->parent = node;
				node->children[0] = Z;
			}

			Z = node->parent;
			node = rotate_right (node);
			node->parent = Z;

			if (Z)
				Z->children[Z->children[1] == Y] = node;
			else
				tree->vma_tree = node;
		} else
			/** No rotation needed.  */
			propagate_height_and_gap (node);

		node = node->parent;
	}
}

/**
 * Insert @node into @tree.
 */
void
vma_tree_insert (struct vma_tree *tree, struct vma_node *node)
{
	node->children[0] = NULL;
	node->children[1] = NULL;
	node->parent = NULL;
	node->height = 1;

	struct vma_node *search = tree->vma_tree;
	if (!search) {
		node->prev_gap = node->first;
		node->biggest_gap = node->first;
		list_insert (&tree->vma_list, &node->vma_list);
		tree->vma_tree = node;
		return;
	}

	int direction = node->first > search->first;
	while (search->children[direction]) {
		search = search->children[direction];
		direction = node->first > search->first;
	}

	struct vma_node *predecessor, *successor;
	if (direction) {
		predecessor = search;
		successor = next_node (tree, search);
	} else {
		predecessor = prev_node (tree, search);
		successor = search;
	}

	if (predecessor)
		node->prev_gap = node->first - predecessor->last - 1;
	else
		node->prev_gap = node->first;

	if (successor)
		successor->prev_gap = successor->first - node->last - 1;

	node->parent = search;
	search->children[direction] = node;
	if (predecessor)
		list_insert (&predecessor->vma_list, &node->vma_list);
	else
		list_insert (&tree->vma_list, &node->vma_list);

	fixup_tree (tree, node);
}

/**
 * Remove @node from @tree.
 */
void
vma_tree_remove (struct vma_tree *tree, struct vma_node *node)
{
	struct vma_node *predecessor, *successor, *Z, *parent = node->parent;

	/** Node deletion changes prev_gap of its successor.  */

	predecessor = prev_node (tree, node);
	successor = next_node (tree, node);
	if (successor && predecessor)
		successor->prev_gap = successor->first - predecessor->last - 1;
	else if (successor)
		successor->prev_gap = successor->first;

	list_delete (&node->vma_list);

	/**
	 * Tree structure:
	 *      before            after
	 *      node              Z
	 *    Z
	 */
	if (!node->children[1]) {
		/** Our right child is NULL.  */
		/** This guarantees that the successor is in the fixup chain. */
		Z = node->children[0];
		if (parent)
			parent->children[parent->children[1] == node] = Z;
		else
			tree->vma_tree = Z;

		if (Z) {
			Z->parent = parent;
			fixup_tree (tree, Z);
		} else
			fixup_tree (tree, parent);

		return;
	}

	/**
	 * Tree structure:
	 *      before            after
	 *      node             (Z
	 *           (Z         ...
	 *          ...       ...)
	 *        ...)        successor
	 *        successor
	 */
	if (!node->children[0]) {
		/** Our successor is contained in the right subtree.  */
		/** Start fixup from the successor. */
		Z = node->children[1];
		if (parent)
			parent->children[parent->children[1] == node] = Z;
		else
			tree->vma_tree = Z;
		Z->parent = parent;

		fixup_tree (tree, successor);
		return;
	}

	/** This is the annoying case.  Replace the node with its successor.  */

	Z = successor;
	struct vma_node *Y = Z->parent;
	Z->parent = parent;
	Z->children[0] = node->children[0];
	Z->children[0]->parent = Z;
	if (parent)
		parent->children[parent->children[1] == node] = Z;
	else
		tree->vma_tree = Z;

	/**
	 * Tree structure:
	 *     before                  after
	 *     node, Y                 Z
	 *   A         Z            A     B
	 *               B
	 */
	if (Y == node) {
		/** The successor is the direct child of @node.  */
		fixup_tree (tree, Z);
		return;
	}

	/**
	 * Tree structure:
	 *    before                   after
	 *    node                     Z
	 *  A          (...         A      (...
	 *            ...)                ...)
	 *          Y                   Y
	 *        Z                   B
	 *          B
	 */

	/** We need to be careful with the right subtree of @node.  */
	Y->children[0] = Z->children[1];
	if (Y->children[0])
		Y->children[0]->parent = Y;
	Z->children[1] = node->children[1];
	Z->children[1]->parent = Z;

	fixup_tree (tree, Y);
}

/**
 * Update biggest_gap after having edited the range that @node covers.
 */
void
vma_tree_adjust (struct vma_tree *tree, struct vma_node *node)
{
	struct vma_node *predecessor = prev_node (tree, node);
	if (predecessor)
		node->prev_gap = node->first - predecessor->last - 1;
	else
		node->prev_gap = node->first;

	struct vma_node *successor = node->children[1];
	if (successor) {
		while (successor->children[0])
			successor = successor->children[0];

		successor->prev_gap = successor->first - node->last - 1;
		node = successor;
	} else {
		successor = next_node (tree, node);
		if (successor)
			successor->prev_gap = successor->first - node->last - 1;
	}

	do {
		propagate_height_and_gap (node);
		node = node->parent;
	} while (node);
}
