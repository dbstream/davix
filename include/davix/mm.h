/**
 * Memory management for userspace.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef __DAVIX_MM_H
#define __DAVIX_MM_H 1

#include <davix/errno.h>
#include <davix/mutex.h>
#include <davix/refcount.h>
#include <davix/sys_types.h>
#include <davix/vma_tree.h>

struct file;
struct process_mm;
struct tlb;
struct vm_area_ops;

#define PROT_NONE		0x00
#define PROT_EXEC		0x01
#define PROT_WRITE		0x02
#define PROT_READ		0x04

#define PROT_VALID_BITS		0x07

#define MAP_SHARED		0x1
#define MAP_PRIVATE		0x2
#define MAP_TYPE_BITS		0xf
#define MAP_ANON		0x0010
#define MAP_FIXED		0x0020
#define MAP_FIXED_NOREPLACE	0x0040

#define VM_EXEC			0x01
#define VM_WRITE		0x02
#define VM_READ			0x04
#define VM_SHARED		0x08

struct vm_area {
	struct process_mm *mm;

	struct vma_node vma_node;

	const struct vm_area_ops *ops;

	void *private;

	unsigned int vm_flags;
};

/**
 * Get the start of @vma. This is the first byte in the vm_area.
 */
static inline unsigned long
vma_start (struct vm_area *vma)
{
	return vma->vma_node.first;
}

/**
 * Get the end of @vma. This is the byte after the last byte in the vm_area.
 */
static inline unsigned long
vma_end (struct vm_area *vma)
{
	return vma->vma_node.last + 1UL;
}

struct vm_pgtable_op;

struct vm_area_ops {
	/**
	 * Open the VMA. This is called when a new VMA has been created, for
	 * example by splitting the VMA for munmap or mmap(MAP_FIXED).
	 *
	 * @vma		The new VMA.  vma->private has been copied from the old
	 *		VMA.
	 *
	 * This function may be NULL.
	 */
	void (*open_vma) (struct vm_area *vma);

	/** 
	 * Close the VMA. This is called after all pages have been unmapped and
	 * the VMA is being removed from the MM.
	 *
	 * @vma		The VMA that is being removed.
	 *
	 * This function may be NULL.
	 */
	void (*close_vma) (struct vm_area *vma);

	/**
	 * Test if it is possible to split the VMA at the given address.
	 *
	 * @vma		The VMA that we are checking if it is possible to split.
	 * @addr	The address at which we want to make the split.
	 *
	 * Returns an errno_t indicating whether it is possible to split at the
	 * given address.
	 *
	 * This function may be NULL.
	 */
	errno_t (*can_split_vma) (struct vm_area *vma, unsigned long addr);

	/**
	 * Notify the VMA that a range of memory has been removed.
	 *
	 * @op		Page table accessor.  This should be passed to PTE
	 *		unmapping functions.
	 * @vma		The VMA that has been unmapped from.
	 * @start	Start of the unmapped region.
	 * @end		End of the unmapped region.
	 *
	 * This function is called when something eg. munmap or mmap(MAP_FIXED)
	 * wants to remove some part of the memory mapping. At this point, @vma
	 * has already been updated accordingly. If the mapping created a hole,
	 * @vma may be any of the surrounding VMAs.
	 *
	 * The purpose of this function is for VM objects to be able to store
	 * information in the actual PTEs, which requires the use of special
	 * functions from vm.
	 *
	 * If this function is NULL, the range will be unmapped but no pages
	 * pointed to by the PTEs will be freed.
	 */
	void (*unmap_vma_range) (struct vm_pgtable_op *op, struct vm_area *vma,
			unsigned long start, unsigned long end);

	/**
	 * Map a range of memory.
	 *
	 * @op		struct vm_pgtable_op - should be passed to any vm_map_*
	 *		function that takes it as an argument.
	 * @vma		The VMA that is being mapped.
	 * @start	Start of the region being mapped.
	 * @end		End of the region being mapped.
	 */
	errno_t(*map_vma_range) (struct vm_pgtable_op *op, struct vm_area *vma,
			unsigned long start, unsigned long end);
};

struct process_mm {
	/**
	 * vma_tree: a collection of vm_areas
	 */
	struct vma_tree vma_tree;

	/** 
	 * This is the start and end region that userspace can map into. Their
	 * values are decided at MM creation time.
	 */
	unsigned long mm_start, mm_end;

	/**
	 * This is the default base for any mmap's, used when no address hint
	 * is given and when !MAP_FIXED.
	 */
	unsigned long mmap_base;

	/**
	 * Reference count. Use mmget and mmput instead.
	 */
	refcount_t refcount;

	/** 
	 * Page tables used for this MM.
	 * (opaque handle used with get_pgtable)
	 */
	void *pgtable_handle;

	/**
	 * MM lock.  Currently a mutex but will be replaced with rwmutex later.
	 * Don't access directly.  Use mm_(un)lock(_shared) instead.
	 */
	struct mutex mm_mutex;
};

extern void
__mmput (struct process_mm *mm);

/**
 * Grab a reference to @mm.
 *
 * Returns @mm itself, to be used in code like:
 *
 *	struct process_mm *mm = mmget (task->mm);
 *
 * Calling mmget() on your own mm is unnecessary unless you have intent to share
 * it with someone else, because each task implicitly references its mm.
 */
static inline struct process_mm *
mmget (struct process_mm *mm)
{
	refcount_inc (&mm->refcount);
	return mm;
}

/**
 * Release a reference to @mm.
 *
 * If the refcount drops to zero, the MM is destroyed.
 */
static inline void
mmput (struct process_mm *mm)
{
	if (refcount_dec (&mm->refcount))
		__mmput (mm);
}

/**
 *	VMA iterators
 *
 * VMA iterators should be used instead of directly using vma_tree. This is
 * because in the future, vma_tree may be replaced by some other solution.
 */
struct vm_area_iter {
	struct vma_tree_iterator it;
};

/**
 * Create a vmi from @mm and @vma.
 */
static inline void
vmi_make (struct vm_area_iter *it, struct process_mm *mm, struct vm_area *vma)
{
	vma_tree_make_iterator (&it->it, &mm->vma_tree, &vma->vma_node);
}

/**
 * Go to the first vma and return true if there is one.
 */
static inline bool
vmi_first (struct vm_area_iter *it, struct process_mm *mm)
{
	return vma_tree_first (&it->it, &mm->vma_tree);
}

/**
 * Go to the last vma and return true if there is one.
 */
static inline bool
vmi_last (struct vm_area_iter *it, struct process_mm *mm)
{
	return vma_tree_last (&it->it, &mm->vma_tree);
}

/**
 * Go to the previous vma and return true if there is one.
 */
static inline bool
vmi_prev (struct vm_area_iter *it)
{
	return vma_tree_prev (&it->it);
}

/**
 * Go to the next vma and return true if there is one.
 */
static inline bool
vmi_next (struct vm_area_iter *it)
{
	return vma_tree_next (&it->it);
}

/**
 * Get the associated vm_area of a vmi.
 */
static inline struct vm_area *
vmi_vm_area (struct vm_area_iter *it)
{
	return struct_parent (struct vm_area, vma_node, vma_node (&it->it));
}

/**
 * Get the previous vma of @vma, or NULL.
 */
static inline struct vm_area *
vm_area_prev (struct process_mm *mm, struct vm_area *vma)
{
	struct vm_area_iter it;
	vmi_make (&it, mm, vma);
	if (vmi_prev (&it))
		return vmi_vm_area (&it);
	return NULL;
}

/**
 * Get the next vma of @vma, or NULL.
 */
static inline struct vm_area *
vm_area_next (struct process_mm *mm, struct vm_area *vma)
{
	struct vm_area_iter it;
	vmi_make (&it, mm, vma);
	if (vmi_next (&it))
		return vmi_vm_area (&it);
	return NULL;
}

/**
 *	The mm lock
 *
 * The mm lock is used for mutual exclusion of writers and readers of a process'
 * address space. For example, page fault handlers should take the lock in
 * shared mode, while mmap, mremap, and munmap should take the lock in exclusive
 * mode.
 *
 * Currently shared and exclusive mode behave equally, since the mm_lock is
 * implemented as a regular mutex.
 */

/**
 * Acquire the mm lock in exclusive mode (for updating the mappings).
 */
static inline errno_t
mm_lock (struct process_mm *mm)
{
	return mutex_lock_interruptible (&mm->mm_mutex);
}

/**
 * Acquire the mm lock in shared mode (for reading the mappings).
 */
static inline errno_t
mm_lock_shared (struct process_mm *mm)
{
	return mutex_lock_interruptible (&mm->mm_mutex);
}

/**
 * Release the mm lock from exclusive mode. If it was locked with
 * mm_lock_shared or mm_downgrade_lock has been called, use
 * mm_unlock_shared instead.
 */
static inline void
mm_unlock (struct process_mm *mm)
{
	mutex_unlock (&mm->mm_mutex);
}

/**
 * Release the mm lock from shared mode. If it was locked with mm_lock
 * and mm_downgrade_lock hasn't been called, use mm_unlock instead.
 */
static inline void
mm_unlock_shared (struct process_mm *mm)
{
	mutex_unlock (&mm->mm_mutex);
}

/**
 * Downgrade a lock held in exclusive mode to shared mode.
 */
static inline void
mm_downgrade_lock (struct process_mm *mm)
{
	(void) mm;
}

/**
 *	Memory manager API
 */

/**
 * Create a new (empty) process address space.
 */
extern struct process_mm *
mmnew (void);

/**
 * Initialize the Davix memory manager.
 */
extern void
mm_init (void);


/**
 * Perform a mmap() syscall.
 *
 * @addr	address hint
 * @length	length
 * @prot	PROT_* flags
 * @flags	MAP_* flags
 * @file	file corresponding to fd
 * @offset	file offset
 */
extern errno_t
ksys_mmap (void *addr, size_t length, int prot, int flags,
		struct file *file, off_t offset, void **out_addr);

/**
 * Perform a munmap() syscall.
 *
 * @addr	address hint
 * @length	lenth
 * @prot	PROT_* flags
 * @flags	MAP_* flags
 */
extern errno_t
ksys_munmap (void *addr, size_t length);

/**
 * Anonymous VMA operations.
 */
extern const struct vm_area_ops anon_vma_ops;

#endif /* __DAVIX_MM_H  */
