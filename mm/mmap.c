/**
 * mmap, mremap, and munmap
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/align.h>
#include <davix/mm.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/sched_types.h>
#include <davix/slab.h>
#include <asm/page_defs.h>
#include "internal.h"

static struct slab *vma_slab;

static inline struct vm_area *
alloc_vm_area (void)
{
	return slab_alloc (vma_slab);
}

static inline void
free_vm_area (struct vm_area *vma)
{
	slab_free (vma);
}

void
mmap_init (void)
{
	vma_slab = slab_create ("vm_area", sizeof (struct vm_area));
	if (!vma_slab)
		panic ("Failed to create vm_area slab!");
}

/**
 * State tracker for munmap().
 */
struct munmap_state {
	unsigned long start;
	unsigned long end;

	/** First and last VMAs that overlap with [start; end) */
	struct vm_area *first_vma, *last_vma;

	/** Preallocated VMA for vma split situations. */
	struct vm_area *split_vma;
};

static errno_t
munmap_prepare (struct process_mm *mm, struct munmap_state *state);

static void
munmap_apply (struct process_mm *mm, struct munmap_state *state,
		struct tlb *tlb);

static void
munmap_cancel (struct munmap_state *state);

static bool
find_free_area (unsigned long *out, struct process_mm *mm, size_t size,
		unsigned long low, unsigned long high)
{
	return vma_tree_find_free_bottomup (out, &mm->vma_tree,
			size, PAGE_SIZE, low, high);
}


/**
 * The guts of mmap().
 *
 * TODO:
 * - Anonymous VMA merging.
 * - Handle !MAP_ANON.
 */
static errno_t
do_mmap_locked (unsigned long addr, size_t length, int prot, int flags,
		struct file *file, off_t offset, void **out_addr)
{
	(void) file;
	(void) offset;

	struct process_mm *mm = get_current_task ()->mm;

	printk ("in do_mmap_locked\n");

	if (!addr && !(flags & MAP_FIXED))
		addr = mm->mmap_base;

	unsigned long low = mm->mm_start, high = mm->mm_end;
	if (addr >= low)
		low = addr;
	else if (flags & MAP_FIXED)
		return ENOMEM;

	if (addr + length - 1UL < addr)
		return ENOMEM;

	if (addr + length - 1UL > high)
		return ENOMEM;

	if (flags & MAP_FIXED)
		high = addr + length - 1UL;

	if ((flags & MAP_FIXED_NOREPLACE) || !(flags & MAP_FIXED)) {
		if (!find_free_area (&addr, mm, length, low, high))
			return ENOMEM;
	}

	printk ("mapping base address: 0x%lx\n", addr);
	printk ("prepping munmap()\n");

	struct munmap_state unmap_state;
	unmap_state.start = addr;
	unmap_state.end = addr + length;
	errno_t e = munmap_prepare (mm, &unmap_state);
	if (e != ESUCCESS)
		return e;

	printk ("allocating vma...\n");

	struct vm_area *vma = alloc_vm_area ();
	if (!vma) {
		munmap_cancel (&unmap_state);
		return ENOMEM;
	}

	/** TODO: handle non-ANON mmap()s  */

	vma->ops = &anon_vma_ops;
	vma->vma_node.first = addr;
	vma->vma_node.last = addr + length - 1UL;
	vma->vm_flags = 0;

	if (prot & PROT_READ)
		vma->vm_flags |= VM_READ;
	if (prot & PROT_WRITE)
		vma->vm_flags |= VM_WRITE;
	if (prot & PROT_EXEC)
		vma->vm_flags |= VM_EXEC;

	if ((flags & MAP_TYPE_BITS) != MAP_PRIVATE)
		vma->vm_flags |= VM_SHARED;

	if (vma->ops->open_vma)
		vma->ops->open_vma (vma);

	struct vm_pgtable_op op;
	vm_init_offline_pgtable_op (&op);

	printk ("mapping range...\n");

	e = vma->ops->map_vma_range (&op, vma, vma_start (vma), vma_end (vma));
	if (e != ESUCCESS) {
		vm_cancel_offline_pgtable_op (&op);
		munmap_cancel (&unmap_state);
		if (vma->ops->close_vma)
			vma->ops->close_vma (vma);
		free_vm_area (vma);
		return e;
	}

	struct tlb tlb;
	tlbflush_init (&tlb, mm);
	printk ("applying munmap...\n");
	munmap_apply (mm, &unmap_state, &tlb);
	printk ("applying offline vm_pgtable_op...\n");
	vm_apply_offline_pgtable_op (&op, mm, &tlb, addr, addr + length);
	printk ("inserting range into vma_tree...\n");
	vma_tree_insert (&mm->vma_tree, &vma->vma_node);
	printk ("flushing tlb...\n");
	tlb_flush (&tlb);

	printk ("do_mmap_locked: done\n");
	*out_addr = (void *) addr;
	return ESUCCESS;
}

errno_t
ksys_mmap (void *addr_, size_t length, int prot, int flags,
		struct file *file, off_t offset, void **out_addr)
{
	if (prot & ~PROT_VALID_BITS)
		return EINVAL;

	switch (flags & MAP_TYPE_BITS) {
	case MAP_SHARED:
	case MAP_PRIVATE:
		break;
	default:
		return EINVAL;
	}

	unsigned long addr = (unsigned long) addr_;

	length = ALIGN_UP (length, PAGE_SIZE);
	if (!length)
		return EINVAL;

	// TODO: implement file mmap()
	if (!(flags & MAP_ANON))
		return ENOTSUP;

	if (flags & MAP_FIXED_NOREPLACE)
		flags |= MAP_FIXED;

	struct process_mm *mm = get_current_task ()->mm;
	if (!mm)
		return EINVAL;

	errno_t e = mm_lock (mm);
	if (e != ESUCCESS)
		return e;

	e = do_mmap_locked (addr, length, prot, flags, file, offset, out_addr);
	mm_unlock (mm);
	return e;
}

static struct vm_area *
find_first_vma_above (struct process_mm *mm, unsigned long addr)
{
	struct vma_tree_iterator it;
	if (vma_tree_find_first (&it, &mm->vma_tree, addr))
		return struct_parent (struct vm_area, vma_node, vma_node (&it));
	return NULL;
}

static errno_t
munmap_prepare (struct process_mm *mm, struct munmap_state *state)
{
	state->first_vma = NULL;
	state->last_vma = NULL;
	state->split_vma = NULL;

	struct vm_area *next, *vma = find_first_vma_above (mm, state->start);
	if (!vma || vma_start (vma) >= state->end)
		return ESUCCESS;

	state->first_vma = vma;
	for (;;) {
		next = vm_area_next (mm, vma);
		if (!next)
			break;
		if (vma_start (next) >= state->end)
			break;
		vma = next;
	}

	state->last_vma = vma;

	errno_t e = ESUCCESS;
	bool split_prev = false, split_next = false;
	vma = state->first_vma;
	if (vma_start (vma) < state->start)
		split_prev = true;
	if (split_prev && vma->ops->can_split_vma)
		e = vma->ops->can_split_vma (vma, state->start);
	if (e != ESUCCESS)
		return e;

	if (!split_prev) {
		vma = vm_area_prev (mm, vma);
		if (vma)
			state->start = vma_end (vma);
		else
			state->start = mm->mm_start;
	}

	vma = state->last_vma;
	if (vma_end (vma) > state->end)
		split_next = true;
	if (split_next && vma->ops->can_split_vma)
		e = vma->ops->can_split_vma (vma, state->end);
	if (e != ESUCCESS)
		return e;

	if (!split_next) {
		vma = vm_area_next (mm, vma);
		if (vma)
			state->end = vma_start (vma);
		else
			state->end = mm->mm_end + 1UL;
	}

	if (state->first_vma == state->last_vma && split_prev && split_next) {
		state->split_vma = alloc_vm_area ();
		if (!state->split_vma)
			return ENOMEM;
	}

	return ESUCCESS;
}

static void
unmap_range (struct vm_pgtable_op *op, struct vm_area *vma,
		unsigned long start, unsigned long end)
{
	if (vma->ops->unmap_vma_range)
		vma->ops->unmap_vma_range (op, vma, start, end);
}

static void
munmap_apply (struct process_mm *mm, struct munmap_state *state,
		struct tlb *tlb)
{
	struct vm_pgtable_op op;
	vm_init_online_pgtable_op (&op, mm, tlb);
	if (state->split_vma) {
		struct vm_area *old_vma = state->first_vma;
		struct vm_area *new_vma = state->split_vma;

		*new_vma = *old_vma;
		old_vma->vma_node.last = state->start - 1UL;
		new_vma->vma_node.first = state->end;

		vma_tree_adjust (&mm->vma_tree, &old_vma->vma_node);
		vma_tree_insert (&mm->vma_tree, &new_vma->vma_node);

		if (new_vma->ops->open_vma)
			new_vma->ops->open_vma (new_vma);

		unmap_range (&op, new_vma, state->start, state->end);
	} else {
		struct vm_area *next, *vma = state->first_vma;
		if (!vma)
			return;
		do {
			if (vma == state->last_vma)
				next = NULL;
			else
				next = vm_area_next (mm, vma);

			unsigned long start = vma_start (vma);
			unsigned long end = vma_end (vma);

			start = max (unsigned long, start, state->start);
			end = min (unsigned long, end, state->end);
			unmap_range (&op, vma, start, end);

			if (vma_start (vma) < start) {
				vma->vma_node.last = start - 1UL;
				vma_tree_adjust (&mm->vma_tree, &vma->vma_node);
			} else if (vma_end (vma) > end) {
				vma->vma_node.first = end;
				vma_tree_adjust (&mm->vma_tree, &vma->vma_node);
			} else {
				if (vma->ops->close_vma)
					vma->ops->close_vma (vma);
				vma_tree_remove (&mm->vma_tree, &vma->vma_node);
				free_vm_area (vma);
			}
			vma = next;
		} while (vma);
	}

	vm_free_pgtable_range (&op, state->start, state->end);
}

static void
munmap_cancel (struct munmap_state *state)
{
	if (state->split_vma)
		free_vm_area (state->split_vma);
}

errno_t
ksys_munmap (void *addr, size_t length)
{
	unsigned long start = (unsigned long) addr;
	unsigned long end = start + length;

	if (end < start)
		return EINVAL;

	if (start & (PAGE_SIZE - 1))
		return EINVAL;

	end = ALIGN_UP (end, PAGE_SIZE);
	if (!end)
		return EINVAL;

	struct process_mm *mm = get_current_task ()->mm;
	if (!mm)
		return EINVAL;

	struct munmap_state state;
	state.start = start;
	state.end = end;

	errno_t e = mm_lock (mm);
	if (e != ESUCCESS)
		return e;

	e = munmap_prepare (mm, &state);
	if (e != ESUCCESS) {
		mm_unlock (mm);
		return e;
	}

	struct tlb tlb;
	tlbflush_init (&tlb, mm);
	munmap_apply (mm, &state, &tlb);
	tlb_flush (&tlb);

	mm_unlock (mm);
	return ESUCCESS;
}

