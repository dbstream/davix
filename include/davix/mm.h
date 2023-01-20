/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_MM_H
#define __DAVIX_MM_H

#include <davix/atomic.h>
#include <davix/mm_types.h>

#define _set_page_bit(page, bit, memorder) \
	atomic_fetch_or(&(page)->flags, (bit), (memorder))

#define _clear_page_bit(page, bit, memorder) \
	atomic_fetch_and(&(page)->flags, ~(bit), (memorder))

#define set_page_bit(page, bit) \
	_set_page_bit((page), (bit), memory_order_relaxed)

#define clear_page_bit(page, bit) \
	_clear_page_bit((page), (bit), memory_order_relaxed)

#define _test_page_bit(page, bit, memorder) \
	(atomic_load(&(page)->flags, (memorder)) & (bit))

#define test_page_bit(page, bit) \
	_test_page_bit((page), (bit), memory_order_relaxed)

extern struct mm kernelmode_mm;

static inline void mm_lock(struct mm *mm)
{
	_spin_acquire(&mm->mm_lock);
}

static inline void mm_unlock(struct mm *mm)
{
	_spin_release(&mm->mm_lock);
}

static inline void vma_lock(struct vm_area *vma)
{
	_spin_acquire(&vma->vma_lock);
}

static inline void vma_unlock(struct vm_area *vma)
{
	_spin_acquire(&vma->vma_lock);
}

static inline struct vm_area *vma_prev(struct mm *mm, struct vm_area *vma)
{
	return vma->list.prev == &mm->vma_list
		? NULL
		: container_of(vma->list.prev, struct vm_area, list);
}

static inline struct vm_area *vma_next(struct mm *mm, struct vm_area *vma)
{
	return vma->list.next == &mm->vma_list
		? NULL
		: container_of(vma->list.next, struct vm_area, list);
}

struct get_unmapped_area_info {
	/*
	 * start..end represents a range of memory which is acceptable.
	 */
	unsigned long start;
	unsigned long end;

	/*
	 * Required alignment.
	 */
	unsigned long align;

	/*
	 * Should we allocate in top-down or bottom-up mode?
	 */
	int topdown;
};

/*
 * Find a free range of size `size` in the specified MM.
 * Return value: start address of the free range or an errno.
 */
unsigned long get_unmapped_area(struct mm *mm,
	unsigned long size, struct get_unmapped_area_info info);

void vma_avl_update_callback(struct avlnode *node);

/*
 * Return value: a kernel vmap area of size `size`, or
 * NULL to indicate an error.
 *
 * `shift` is the hugepage shift. The allocation will be
 * aligned to `PAGE_SIZE << shift` bytes.
 */
struct vm_area *alloc_vmap_area(unsigned long size, unsigned shift);

/*
 * Free a vmap area allocated by `alloc_vmap_area()`.
 */
void free_vmap_area(struct vm_area *area);

void dump_kernel_vmap_areas(void);

void *vmap(unsigned long phys_addr, unsigned long len,
	pgmode_t mode, pgcachemode_t cachemode);

void vunmap(void *mem);

#endif /* __DAVIX_MM_H */
