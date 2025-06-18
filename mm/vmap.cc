/**
 * Kernel virtual address space management.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/pgtable_modify.h>
#include <davix/kmalloc.h>
#include <davix/page.h>
#include <davix/spinlock.h>
#include <davix/vmap.h>
#include <dsl/align.h>
#include <dsl/minmax.h>
#include <dsl/vmatree.h>

#include <davix/printk.h>

struct vmap_area {
	dsl::VMANode node;
};

typedef dsl::TypedVMATree<vmap_area, &vmap_area::node> VmapTree;

static spinlock_t vmap_lock;
static VmapTree vmap_tree;

static void
__free_pte_range (uintptr_t start, uintptr_t end,
		bool free_pages,
		int level, pte_t *pte,
		tlb_accumulator *tlb)
{
	pte_t value = pte_read (pte);
	if (value.empty ())
		return;

	if (!level) {
		pte_clear (pte);
		tlb_add_range (tlb, start, start + PAGE_SIZE);
		if (free_pages)
			free_page (phys_to_page (value.phys_addr ()));
		return;
	}

	uintptr_t size = __pgtable_entry_size(level + 1);
	bool full = start == dsl::align_down (start, size)
			&& end == dsl::align_up (end, size);

	pte_t *entry = pgtable_entry (value, start, level);
	do {
		uintptr_t next = pgtable_boundary_next (start, end, level);
		__free_pte_range (start, next, free_pages, level - 1, entry, tlb);
		start = next;
		entry++;
	} while (start != end);

	if (full && level < max_pgtable_level () - 1) {
		pte_clear (pte);
		tlb_add_pgtable (tlb, pte_pgtable (value), level);
	}
}

static void
free_pte_range (uintptr_t start, uintptr_t end,
		uintptr_t floor, uintptr_t ceiling,
		bool free_pages)
{
	int level = max_pgtable_level ();

	uintptr_t tmp = dsl::align_down (start, __pgtable_entry_size (level));
	floor = dsl::max (floor, tmp);
	tmp = dsl::align_up (end, __pgtable_entry_size (level));
	if (tmp)
		ceiling = ceiling ? dsl::min (ceiling, tmp) : tmp;

	tlb_accumulator tlb;
	tlb_begin_kernel (&tlb);

	pte_t *entry = get_vmap_pgtable_entry (floor);
	do {
		uintptr_t next = pgtable_boundary_next (floor, ceiling, level);
		__free_pte_range (floor, next, free_pages, level - 1, entry, &tlb);
		floor = next;
		entry++;
	} while (floor != ceiling);

	tlb_end_kernel (&tlb);
}

static void
free_pte_range_vma (uintptr_t start, uintptr_t end,
		vmap_area *vma, bool free_pages)
{
	vmap_area *prev = vmap_tree.prev (vma);
	vmap_area *next = vmap_tree.next (vma);

	free_pte_range (start, end,
			prev ? (prev->node.last + 1) : 0,
			next ? (next->node.first) : 0,
			free_pages);
}

static void
free_pte_range_entire_vma (vmap_area *vma, bool free_pages)
{
	free_pte_range_vma (vma->node.first, vma->node.last + 1, vma, free_pages);
}

void
vunmap (void *ptr)
{
	vmap_lock.lock_dpc ();
	vmap_area *vma = vmap_tree.find ((uintptr_t) ptr);
	if (!vma) {
		vmap_lock.unlock_dpc ();
		printk (PR_ERROR "vunmap() was called on a pointer which does not exist in the vmap_tree\n");
		return;
	}

	free_pte_range_entire_vma (vma, false);
	vmap_tree.remove (vma);
	vmap_lock.unlock_dpc ();
	kfree (vma);
}

void
kfree_large (void *ptr)
{
	vmap_lock.lock_dpc ();
	vmap_area *vma = vmap_tree.find ((uintptr_t) ptr);
	if (!vma) {
		vmap_lock.unlock_dpc ();
		printk (PR_ERROR "kfree_large() was called on a pointer which does not exist in the vmap_tree\n");
		return;
	}

	free_pte_range_entire_vma (vma, true);
	vmap_tree.remove (vma);
	vmap_lock.unlock_dpc ();
	kfree (vma);
}

static pte_t *
get_pte (uintptr_t addr)
{
	int level = max_pgtable_level ();
	pte_t *entry = get_vmap_pgtable_entry (addr);
	do {
		level--;
		pte_t value = pte_read (entry);
		if (value.empty ()) {
			pte_t *new_table = alloc_pgtable (level);
			if (!new_table)
				return nullptr;
			value = make_pte_pgtable_k (new_table);
			if (!pgtable_install (entry, value))
				free_pgtable (new_table, level);
		}
		entry = pgtable_entry (value, addr, level);
	} while (level > 1);
	return entry;
}

static bool
find_free_with_guard_pages (uintptr_t *out, size_t size, uintptr_t low, uintptr_t high)
{
	constexpr size_t left_guard_hole = PAGE_SIZE;
	constexpr size_t right_guard_hole = PAGE_SIZE;

	size_t hole_size = size + left_guard_hole + right_guard_hole;
	if (hole_size < size)
		return false;

	if (low > left_guard_hole)
		low -= left_guard_hole;
	else
		low = 0;

	if (high < dsl::VMA_TREE_MAX - right_guard_hole)
		high += right_guard_hole;
	else
		high = dsl::VMA_TREE_MAX;

	uintptr_t addr = 0;
	if (!vmap_tree.find_free_bottomup (&addr, hole_size, PAGE_SIZE, low, high))
		return false;

	*out = addr + left_guard_hole;
	return true;
}

/**
 * vmap_io_range - map memory-mapped IO into the kernel's address space.
 * @phys: physical address of MMIO window
 * @size: size of MMIO window in bytes
 * @pcm: page cache mode
 * @low: lowest acceptable mapping address
 * @high: highest acceptable mapping address
 * Returns NULL on failure.
 */
void *
vmap_io_range (uintptr_t phys, size_t size, page_cache_mode pcm, uintptr_t low, uintptr_t high)
{
	uintptr_t offset_in_page = phys & (PAGE_SIZE - 1);
	if (size + offset_in_page < size) return nullptr;

	phys -= offset_in_page;
	size += offset_in_page;
	size = dsl::align_up (size, PAGE_SIZE);
	if (!size)
		return nullptr;

	vmap_area *vma = (vmap_area *) kmalloc (sizeof (*vma), ALLOC_KERNEL);
	if (!vma)
		return nullptr;

	uintptr_t addr = 0;

	vmap_lock.lock_dpc ();
	if (!find_free_with_guard_pages (&addr, size, low, high)) {
		vmap_lock.unlock_dpc ();
		kfree (vma);
		return nullptr;
	}

	asm ("" : "+r"(addr));
	vma->node.first = addr;
	vma->node.last = addr + size - 1;
	vmap_tree.insert (vma);
	vmap_lock.unlock_dpc ();

	for (uintptr_t i = 0; i < size; i += PAGE_SIZE) {
		pte_t *pte = get_pte (addr + i);
		if (!pte) {
			vmap_lock.lock_dpc ();
			free_pte_range_vma (addr, addr + i, vma, false);
			vmap_tree.remove (vma);
			vmap_lock.unlock_dpc ();
			kfree (vma);
			return nullptr;
		}

		pte_install (pte, make_io_pte (phys + i, pcm));
	}

	return (void *) (vma->node.first + offset_in_page);
}

/**
 * vmap_io - map memory-mapped IO into the kernel's address space.
 * @phys: physical address of MMIO window
 * @size: size of MMIO window in bytes
 * @pcm: page cache mode
 * Returns NULL on failure.
 */
void *
vmap_io (uintptr_t phys, size_t size, page_cache_mode pcm)
{
	return vmap_io_range (phys, size, pcm, KERNEL_VM_FIRST, KERNEL_VM_LAST);
}

/**
 * vmap_range - map physical memory into the kernel's address space.
 * @phys: physical address of memory
 * @size: size of memory in bytes
 * @low: lowest acceptable mapping address
 * @high: highest acceptable mapping address
 * Returns NULL on failure.
 */
void *
vmap_range (uintptr_t phys, size_t size, uintptr_t low, uintptr_t high)
{
	return vmap_io_range (phys, size, PCM_NORMAL_RAM, low, high);
}

/**
 * vmap - map physical memory into the kernel's address space.
 * @phys: physical address of memory
 * @size: size of memory in bytes
 * Returns NULL on failure.
 */
void *
vmap (uintptr_t phys, size_t size)
{
	return vmap_io_range (phys, size, PCM_NORMAL_RAM, KERNEL_VM_FIRST, KERNEL_VM_LAST);
}

/**
 * kmalloc_large - perform a large kmalloc allocation using vmap.
 * @size: size of the allocation in bytes
 * Returns NULL on failure.
 */
void *
kmalloc_large (size_t size)
{
	size = dsl::align_up (size, PAGE_SIZE);
	if (!size)
		return nullptr;

	vmap_area *vma = (vmap_area *) kmalloc (sizeof (*vma), ALLOC_KERNEL);
	if (!vma)
		return nullptr;

	uintptr_t addr = 0;

	vmap_lock.lock_dpc ();
	if (!find_free_with_guard_pages (&addr, size, KERNEL_VM_FIRST, KERNEL_VM_LAST)) {
		vmap_lock.unlock_dpc ();
		kfree (vma);
		return nullptr;
	}

	asm ("" : "+r"(addr));
	vma->node.first = addr;
	vma->node.last = addr + size - 1;
	vmap_tree.insert (vma);
	vmap_lock.unlock_dpc ();

	for (uintptr_t i = 0; i < size; i += PAGE_SIZE) {
		Page *page = alloc_page (ALLOC_KERNEL);
		if (!page) {
			vmap_lock.lock_dpc ();
			free_pte_range_vma (addr, addr + i, vma, true);
			vmap_tree.remove (vma);
			vmap_lock.unlock_dpc ();
			kfree (vma);
			return nullptr;
		}

		pte_t *pte = get_pte (addr + i);
		if (!pte) {
			free_page (page);
			vmap_lock.lock_dpc ();
			free_pte_range_vma (addr, addr + i, vma, true);
			vmap_tree.remove (vma);
			vmap_lock.unlock_dpc ();
			kfree (vma);
			return nullptr;
		}

		pte_install (pte, make_pte_k (page_to_phys (page), PAGE_KERNEL_DATA));
	}

	return (void *) vma->node.first;
}
