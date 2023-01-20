/* SPDX-License-Identifier: MIT */
/*
 * mmap.c - managing the VMA tree.
 */
#include <davix/errno.h>
#include <davix/mm.h>
#include <asm/page.h>

static unsigned long get_unmapped_area_bottomup(struct mm *mm,
	unsigned long size, struct get_unmapped_area_info info)
{
	if(mm->vma_tree.root == NULL)
		return info.start;

	struct vm_area *vma = vma_of(mm->vma_tree.root);
	for(;;) {
		unsigned long hole_end = vma->start;
		/* Can there be any suitable holes to the left? */
		if(vma->avl.left && hole_end >= info.start + size) {
			struct vm_area *left = vma_of(vma->avl.left);

			/* Are they big enough? */
			if(left->biggest_gap_to_prev >= size) {
				/* Continue descending to the left. */
				vma = left;
				continue;
			}
		}

check_current_node:;
		struct vm_area *prev = vma_prev(mm, vma);
		unsigned long hole_start = prev ? prev->end : 0;

		/* If this hole is too high, return immediately. */
		if(hole_start > info.end - size)
			return -ENOMEM;

		/* Align the start and clamp the start and end of the hole. */
		hole_start = max(align_up(hole_start, info.align), info.start);
		hole_end = min(hole_end, info.end);

		if(hole_end > hole_start && hole_end - hole_start >= size)
			/* We found ourself a suitable hole! */
			return hole_start;

		/* Can there be any suitable holes to the right? */
		if(vma->avl.right) {
			struct vm_area *right = vma_of(vma->avl.right);

			/* Are they big enough? */
			if(right->biggest_gap_to_prev >= size) {
				/* Search the right subtree. */
				vma = right;
				continue;
			}
		}

		/* None of our child nodes are suitable. Go up the tree. */
		for(;;) {
			if(!vma->avl.parent) {
				/*
				 * We are the root node, and thus cannot go up
				 * the tree.
				 *
				 * Check if there is room after the last VMA.
				 * If there is not, terminate the search.
				 */
				prev = container_of(mm->vma_list.prev,
					struct vm_area, list);
				hole_start =
					max(align_up(prev->end, info.align),
						info.start);
				hole_end = info.end;
				if(hole_end > hole_start
					&& hole_end - hole_start >= size)
						return hole_start;

				return -ENOMEM;
			}
			struct vm_area *old = vma;
			vma = vma_of(vma->avl.parent);

			/* If we are the left child, break out of this loop. */
			if(old == vma_of(vma->avl.left)) {
				goto check_current_node;
			}
		}
	}
}

static unsigned long get_unmapped_area_topdown(struct mm *mm,
	unsigned long size, struct get_unmapped_area_info info)
{
	if(mm->vma_tree.root == NULL)
		return align_down(info.end - size, info.align);

	struct vm_area *vma =
		container_of(mm->vma_list.prev, struct vm_area, list);
	unsigned long hole_start =
		max(align_up(vma->end, info.align), info.start);
	unsigned long hole_end = info.end;
	if(hole_end > hole_start && hole_end - hole_start >= size)
		return align_down(hole_end - size, info.align);

	vma = vma_of(mm->vma_tree.root);
	for(;;) {
		/* Can we go into the right subtree? */
		if(vma->avl.right && vma->end <= info.end - size) {
			struct vm_area *right = vma_of(vma->avl.right);
			/* Are there any holes that are big enough? */
			if(right->biggest_gap_to_prev >= size) {
				vma = right;
				continue;
			}
		}
check_current_node:;
		hole_end = vma->start;
		/* If the hole is too low, return immediately. */
		if(hole_end < info.start + size)
			return -ENOMEM;

		struct vm_area *prev = vma_prev(mm, vma);
		hole_start = prev ? prev->end : 0;

		/* Align the start and clamp the start and end. */
		hole_start = max(align_up(hole_start, info.align), info.start);
		hole_end = min(hole_end, info.end);
		if(hole_end > hole_start && hole_end - hole_start >= size)
			/* We found ourselves a suitable hole! */
			return align_down(hole_end - size, info.align);

		/* Can there be any suitable holes to our left? */
		if(vma->avl.left) {
			struct vm_area *left = vma_of(vma->avl.left);
			/* Are they big enough? */
			if(left->biggest_gap_to_prev >= size) {
				vma = left;
				continue;
			}
		}

		/* We need to go up the tree. */
		for(;;) {
			if(!vma->avl.parent)
				/* We are the root node. End the search. */
				return -ENOMEM;

			struct vm_area *old = vma;
			vma = vma_of(vma->avl.parent);
			if(old == vma_of(vma->avl.right))
				goto check_current_node;
		}
	}
}

unsigned long get_unmapped_area(struct mm *mm,
	unsigned long size, struct get_unmapped_area_info info)
{
	info.align = max(info.align, PAGE_SIZE);
	info.start = align_up(info.start, info.align);

	if(info.end <= info.start)
		return -ENOMEM;

	if(info.end - info.start < size)
		return -ENOMEM;

	return info.topdown
		? get_unmapped_area_topdown(mm, size, info)
		: get_unmapped_area_bottomup(mm, size, info);
}

void vma_avl_update_callback(struct avlnode *node)
{
	struct vm_area *vma = vma_of(node);
	struct vm_area *prev = vma_prev(vma->mm, vma);
	unsigned long prev_end = prev ? prev->end : 0;

	unsigned long gap = vma->start - prev_end;
	if(vma->avl.left) {
		struct vm_area *left = vma_of(vma->avl.left);
		if(left->biggest_gap_to_prev > gap)
			gap = left->biggest_gap_to_prev;
	}
	if(vma->avl.right) {
		struct vm_area *right = vma_of(vma->avl.right);
		if(right->biggest_gap_to_prev > gap)
			gap = right->biggest_gap_to_prev;
	}

	vma->biggest_gap_to_prev = gap;
}
