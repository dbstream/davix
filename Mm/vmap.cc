// SPDX-License-Identifier: GPL-3.0
/*
 * File: Mm/vmap.cc
 * Kernel virtual mapping functions.
 *
 * Copyright (C) 2025  dbstream
 */
#include "Mm/pfn.h"
#include <Hal/page_tables.h>
#include <Hal/tlb.h>
#include <Ke/spinlock.h>
#include <Mm/page_alloc.h>
#include <Mm/pool.h>
#include <Mm/vmap.h>
#include <asm/clear_page.h>
#include <davix/bug.h>
#include <dsl/vmatree.h>
#include <lock_guard.h>

static void clear_pte_range(TLB_DATA *tlb, int level,
		unsigned long start, unsigned long end,
		bool free_pages)
{
	unsigned long entry_size = HalPTESize(level);

	MMPTEP ptep = MmGetPteForAddressLevel(start, level);
	do {
		unsigned long next = (start + entry_size) & ~(entry_size - 1);
		if (!next || (end && end < next))
			next = end;

		MMPTE pte = HalReadPTE(ptep);
		/*
		 * NOTE: Only call HalPTEAddress inside the conditionals when
		 * needed. Calling HalPTEAddress invokes a BUG() if the PTE is
		 * a huge page.
		 */
		if (!HalPTEEmpty(pte) && HalPTEHuge(pte, level)) {
			HalClearPTE(ptep);
			HalFlushTLB(tlb, start);
			if (free_pages) {
				BUG_ON(level != 1);
				MMPFN *pfn = MmGetPFNForPhys(
					HalPTEAddress(pte, level)
				);
				MmFreePage(pfn);
			}
		} else if (!HalPTEEmpty(pte)) {
			MMPFN *pfn = MmGetPFNForPhys(HalPTEAddress(pte, level));
			clear_pte_range(tlb, level - 1, start, next, free_pages);
			if (!((start | next) & (entry_size - 1))) {
				HalClearPTE(ptep);
				HalFlushAndFreeTableTLB(tlb, level, start, pfn);
			}
		}

		ptep++;
		start = next;
	} while (start != end);
}

static bool populate_tables(unsigned long start, unsigned long end, int level,
		unsigned long floor, unsigned long ceiling)
{
	bool allocated_any = false;

	int current = HalNumPageTableLevels();
	while(current > level) {
		MMPTEP ptep = MmGetPteForAddressLevel(start, current);
		MMPTEP pend = MmGetPteForAddressLevel(end - 1, current) + 1;

		do {	/*
			 * Iterate over page table entries and populate them
			 * with empty page tables as needed.
			 */
			MMPTE pte = HalReadPTE(ptep);
			if (HalPTEEmpty(pte)) {
				MMPFN *pfn = MmAllocatePage();
				if (!pfn) {
					[[unlikely]];
					if (!allocated_any)
						return false;
					TLB_DATA tlb;
					HalBeginTLB(&tlb, nullptr);
					clear_pte_range(&tlb,
							HalNumPageTableLevels(),
							floor, ceiling, false);
					HalEndTLB(&tlb);
					return false;
				}
				allocated_any = true;
				unsigned long phy = MmGetPhysForPFN(pfn);
				clear_page((void *) MiPhysToVirt(phy));
				pte = HalMakeTableKPTE(current, phy);
				HalWritePTE(ptep, pte);
			} else
				BUG_ON(HalPTEHuge(pte, current));
			ptep++;
		} while (ptep != pend);
		current--;
	}

	return true;
}

struct vmap_area {
	dsl::VMANode node;
};

static dsl::TypedVMATree<vmap_area, &vmap_area::node> vmap_tree;
static KeSpinlock vmap_lock;

static void do_unmap(void *mem, bool free_pages)
{
	scoped_lock_guard guard(vmap_lock);

	vmap_area *vma = vmap_tree.find((uintptr_t) mem);
	BUG_ON(!vma);

	unsigned long start = vma->node.first;
	unsigned long end = vma->node.last;
	unsigned long entry_size = HalPTESize(HalNumPageTableLevels() - 1);

	start = start & ~(entry_size - 1);
	end = (end + entry_size - 1) & ~(entry_size - 1);

	vmap_area *prev = vmap_tree.prev(vma);
	vmap_area *next = vmap_tree.next(vma);
	if (prev && prev->node.last + 1UL > start)
		start = prev->node.last + 1UL;
	if (next && next->node.first < end)
		end = next->node.first;

	BUG_ON(start < MiVmapSpaceBegin);
	BUG_ON(end > MiVmapSpaceEnd);
	BUG_ON(start >= end);

	TLB_DATA tlb;
	HalBeginTLB(&tlb, nullptr);
	clear_pte_range(&tlb, HalNumPageTableLevels() - 1, start, end, free_pages);
	HalEndTLB(&tlb);

	vmap_tree.remove(vma);
}

/**
 * MmUnmapVirtual - unmap a memory region mapped by MmMapVirtual.
 * @mem: pointer into region returned by MmMapVirtual
 */
void MmUnmapVirtual(void *mem)
{
	do_unmap(mem, false);
}

/**
 * MmFreeVirtual - free an object allocated by MmAllocateVirtual.
 * @mem: pointer into region returned by MmAllocateVirtual
 */
void MmFreeVirtual(void *mem)
{
	do_unmap(mem, true);
}

static inline bool find_free_address(unsigned long *out,
		unsigned long size, unsigned long align,
		unsigned long low, unsigned long high)
{
	constexpr unsigned long guard_left = PAGE_SIZE;
	constexpr unsigned long guard_right = PAGE_SIZE;
	/*
	 * Alignment must be a power-of-two.
	 */
	BUG_ON(align & (align - 1));

	unsigned long hole_size = size + guard_left + guard_right;
	if (hole_size < size)
		return false;

	if (align <= PAGE_SIZE) {
		/*
		 * Fastpath for unaligned allocations:
		 */
		if (low < guard_left)
			low = 0;
		else
			low -= guard_left;

		if (high > dsl::VMA_TREE_MAX - guard_right)
			high = dsl::VMA_TREE_MAX;
		else
			high += guard_right;

		unsigned long addr = 0;
		bool status = vmap_tree.find_free_bottomup(&addr, hole_size,
				PAGE_SIZE, low, high);

		if (status)
			addr += guard_left;
		*out = addr;
		return status;
	}

	if (low < guard_left)
		low = guard_left;
	if (high > dsl::VMA_TREE_MAX - guard_right)
		high = dsl::VMA_TREE_MAX - guard_right;
	/*
	 * Slowpath used when alignment is required:
	 */
	for (;;) {
		/*
		 * Find a free area that meets the alignment requirements.
		 */
		unsigned long addr = 0;
		bool status = vmap_tree.find_free_bottomup(&addr, size, align,
				low, high);
		if (!status)
			// Game over.
			return false;
		/*
		 * Look for overlapping regions within the guard pages.
		 */
		struct vmap_area *vma = vmap_tree.find_above(addr - guard_left);
		if (!vma || vma->node.first > addr + size + guard_right - 1UL) {
			/*
			 * No conflicting region: this address is suitable.
			 */
			*out = addr;
			return true;
		}
		/*
		 * Ok, we overlap with a memory region in a guard hole.
		 */
		low = vma->node.last + guard_left;
		if (low < vma->node.last)
			// Guard against overflow.
			return false;
	}
}

/**
 * MmMapVirtual - map physical memory into the kernel's virtual address space.
 * @phys: start of the region to map
 * @size: size in bytes of the region to map
 * @flags: PTEFLAGS representing the requested permissions and cache mode
 *
 * Both @phys and @size must be page aligned.  This function will use huge pages
 * whenever possible.
 */
void *MmMapVirtual(unsigned long phys, unsigned long size, PTEFLAGS flags)
{
	if (!size)
		return nullptr;

	if ((size & (PAGE_SIZE - 1)) || (phys & (PAGE_SIZE - 1)))
		return nullptr;

	int level = HalMaxHugePTELevel();

	/*
	 * Try to map memory with huge pages whenever possible (prefer bigger
	 * page sizes).
	 */
	while (level > 1) {
		unsigned long entry_size = HalPTESize(level);
		if (!(size & (entry_size - 1)) && !(phys & (entry_size - 1)))
			break;
		level--;
	}

	vmap_area *vma = MmNew<vmap_area>();
	if (!vma)
		return nullptr;

	scoped_lock_guard guard(vmap_lock);

	unsigned long align = HalPTESize(level);
	unsigned long addr = 0;
	bool status = find_free_address(&addr, size, align,
			MiVmapSpaceBegin, MiVmapSpaceEnd);
	if (!status) {
		guard.drop();
		MmDelete(vma);
		return nullptr;
	}

	vma->node.first = addr;
	vma->node.last = addr + size - 1;

	align = HalPTESize(HalNumPageTableLevels() - 1);
	unsigned long floor = addr & ~(align - 1);
	unsigned long ceiling = (addr + size + align - 1) & ~(align - 1);

	vmap_area *tmp = vmap_tree.find_below(addr);
	if (tmp && tmp->node.last + 1UL > floor)
		floor = tmp->node.last + 1UL;
	tmp = vmap_tree.find_above(addr);
	if (tmp && tmp->node.first < ceiling)
		ceiling = tmp->node.first;

	if (!populate_tables(addr, addr + size, level, floor, ceiling)) {
		guard.drop();
		MmDelete(vma);
		return nullptr;
	}

	vmap_tree.insert(vma);
	guard.drop();

	align = HalPTESize(level);
	MMPTEP ptep = MmGetPteForAddressLevel(addr, level);
	do {
		MMPTE pte = HalMakeKPTE(level, phys, flags);
		HalWritePTE(ptep, pte);

		ptep++;
		phys += align;
		size -= align;
	} while (size != 0);

	return (void *) addr;
}

/**
 * MmAllocateVirtual - allocate kernel objects in virtually contiguous memory.
 * @size: number of bytes to allocate
 * @flags: requested PTEFLAGS
 */
void *MmAllocateVirtual(unsigned long size, PTEFLAGS flags)
{
	size = PGALIGN_UP(size);
	if (!size)
		return nullptr;

	vmap_area *vma = MmNew<vmap_area>();
	if (!vma)
		return nullptr;

	scoped_lock_guard guard(vmap_lock);

	unsigned long addr = 0;
	bool status = find_free_address(&addr, size, PAGE_SIZE,
			MiVmapSpaceBegin, MiVmapSpaceEnd);
	if (!status) {
		guard.drop();
		MmDelete(vma);
		return nullptr;
	}

	vma->node.first = addr;
	vma->node.last = addr + size - 1;

	unsigned long align = HalPTESize(HalNumPageTableLevels() - 1);
	unsigned long floor = addr & ~(align - 1);
	unsigned long ceiling = (addr + size + align - 1) & ~(align - 1);

	vmap_area *tmp = vmap_tree.find_below(addr);
	if (tmp && tmp->node.last + 1UL > floor)
		floor = tmp->node.last + 1UL;
	tmp = vmap_tree.find_above(addr);
	if (tmp && tmp->node.first < ceiling)
		ceiling = tmp->node.first;

	if (!populate_tables(addr, addr + size, 1, floor, ceiling)) {
		guard.drop();
		MmDelete(vma);
		return nullptr;
	}

	MMPTEP ptep = MmGetPteForAddress(addr);
	do {
		MMPFN *pfn = MmAllocatePage();
		if (!pfn) {
			TLB_DATA tlb;
			HalBeginTLB(&tlb, nullptr);
			clear_pte_range(&tlb, HalNumPageTableLevels(),
					floor, ceiling, true);
			HalEndTLB(&tlb);
			guard.drop();
			MmDelete(vma);
			return nullptr;
		}

		MMPTE pte = HalMakeKPTE(1, MmGetPhysForPFN(pfn), flags);
		HalWritePTE(ptep, pte);

		ptep++;
		size -= PAGE_SIZE;
	} while (size != 0);

	vmap_tree.insert(vma);
	return (void *) addr;
}

