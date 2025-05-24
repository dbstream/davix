/**
 * Lookup operations on VMA trees.
 * Copyright (C) 2025-present  dbstream
 */
#include <dsl/align.h>
#include <dsl/minmax.h>
#include <dsl/vmatree.h>

namespace dsl {

/** Find the node which contains addr.  */
VMANode *
VMATree::find (uintptr_t addr)
{
	VMANode *current = root;

	while (current) {
		if (addr < current->first)
			current = current->child[LEFT];
		else if (addr > current->last)
			current = current->child[RIGHT];
		else
			break;
	}

	return current;
}

/** Find the first node on or above addr.  */
VMANode *
VMATree::find_above (uintptr_t addr)
{
	VMANode *current = root, *found = nullptr;

	while (current) {
		if (addr > current->last) {
			current = current->child[RIGHT];
			continue;
		}

		found = current;
		current = current->child[LEFT];
	}

	return found;
}

/** Find the first node below addr.  */
VMANode *
VMATree::find_below (uintptr_t addr)
{
	VMANode *current = root, *found = nullptr;

	while (current) {
		if (addr <= current->first) {
			current = current->child[LEFT];
			continue;
		}

		found = current;
		current = current->child[RIGHT];
	}

	return found;
}

/** Find the first free hole.  */
bool
VMATree::find_free_bottomup (uintptr_t *out,
		uintptr_t size, uintptr_t align,
		uintptr_t min_addr, uintptr_t max_addr)
{
	if (!size)
		return false;

	if (max_addr < min_addr || max_addr < size)
		return false;

	if ((max_addr != VMA_TREE_MAX || min_addr != 0) && max_addr - min_addr + 1 < size)
		return false;

	VMANode *tmp, *current = root;
	uintptr_t alloc_start, gap_start, gap_end;

	if (!current)
		goto look_at_rightmost;

	for (;;) {
		/* If no gap in the current subtree is big enough, skip it. */
		if (current->biggest_gap < size)
			goto go_up;

		/* Do not look at the left subtree if it is below min_addr. */
		tmp = current->child[LEFT];
		if (tmp && current->first > min_addr) {
			current = tmp;
			continue;
		}

look_at_current:
		tmp = prev (current);
		if (tmp)
			gap_start = tmp->last + 1;
		else
			gap_start = 0;
		gap_end = current->first;

		gap_start = max (gap_start, min_addr);
		if (max_addr != VMA_TREE_MAX)
			gap_end = min (gap_end, max_addr + 1);

		alloc_start = dsl::align_up (gap_start, align);
		if (alloc_start < gap_start || gap_end <= alloc_start)
			goto look_at_right_subtree;

		/* Do not keep looking for free areas above max_addr. */
		if (gap_end >= max_addr)
			return false;

look_at_right_subtree:
		tmp = current->child[RIGHT];
		if (tmp) {
			current = tmp;
			continue;
		}

go_up:
		tmp = current;
		current = current->parent;
		if (!current)
			break;

		if (current->child[RIGHT] == tmp)
			goto go_up;

		goto look_at_current;
	}

look_at_rightmost:
	/* Look at the gap after the rightmost node in the tree. */
	current = last ();
	if (current) {
		if (current->last == VMA_TREE_MAX)
			return false;
		gap_start = current->last + 1;
	} else
		gap_start = 0;

	gap_start = max (gap_start, min_addr);
	alloc_start = align_up (gap_start, align);
	if (alloc_start < gap_start)
		return false;

	gap_end = alloc_start + size - 1;
	if (gap_end < alloc_start || gap_end > max_addr)
		return false;

	*out = alloc_start;
	return true;
}

/** Find the last free hole.  */
bool
VMATree::find_free_topdown (uintptr_t *out,
		uintptr_t size, uintptr_t align,
		uintptr_t min_addr, uintptr_t max_addr)
{
	if (!size)
		return false;

	if (max_addr < min_addr || max_addr < size)
		return false;

	if ((max_addr != VMA_TREE_MAX || min_addr != 0) && max_addr - min_addr + 1 < size)
		return false;

	VMANode *tmp, *current = last ();
	uintptr_t alloc_start, gap_start, gap_end;

	if (current) {
		if (current->last == VMA_TREE_MAX)
			goto start_loop;
		gap_start = current->last + 1;
	} else
		gap_start = 0;

	gap_start = max (gap_start, min_addr);
	alloc_start = align_up (gap_start, align);
	if (alloc_start < gap_start)
		goto start_loop;

	gap_end = alloc_start + size - 1;
	if (gap_end >= alloc_start && gap_end <= max_addr) {
		*out = align_down (max_addr + 1 - size, align);
		return true;
	}

start_loop:
	current = root;
	if (!current)
		return false;

	for (;;) {
		/* If no gap in the current subtree is big enough, skip it. */
		if (current->biggest_gap < size)
			goto go_up;

		/* Do not look at the right subtree if it is above max_addr. */
		tmp = current->child[RIGHT];
		if (tmp && current->last < max_addr) {
			current = tmp;
			continue;
		}

look_at_current:
		tmp = prev (current);
		if (tmp)
			gap_start = tmp->last + 1;
		else
			gap_start = 0;
		gap_end = current->first;

		gap_start = max (gap_start, min_addr);
		if (max_addr != VMA_TREE_MAX)
			gap_end = min (gap_end, max_addr + 1);

		alloc_start = align_up (gap_start, align);
		if (alloc_start < gap_start || gap_end <= alloc_start)
			goto look_at_left_subtree;

		if (gap_end - alloc_start >= size) {
			*out = align_down (gap_end - size, align);
			return true;
		}

		/* Do not keep looking for free areas below min_addr. */
		if (gap_start <= min_addr)
			return false;

look_at_left_subtree:
		if (current->child[LEFT]) {
			current = current->child[LEFT];
			continue;
		}

go_up:
		tmp = current;
		current = current->parent;
		if (!current)
			return false;

		if (current->child[LEFT] == tmp)
			goto go_up;

		goto look_at_current;
	}
}

}
