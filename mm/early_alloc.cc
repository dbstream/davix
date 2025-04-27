/**
 * Early memory allocator.
 * Copyright (C) 2025-present  dbstream
 *
 * This is a simple first-fit freelist allocator, suitable for early memory
 * management before the normal page allocation mechanism has been brought
 * online.
 */
#include <asm/zone.h>
#include <davix/early_alloc.h>
#include <davix/page.h>
#include <dsl/align.h>
#include <dsl/list.h>
#include <dsl/minmax.h>
#include <new>

struct early_free_block {
	dsl::ListHead linkage;
	size_t size;
};

constexpr size_t minalign = 4 * sizeof (void *);
static_assert (minalign >= sizeof (early_free_block));

static dsl::TypedList<early_free_block, &early_free_block::linkage> free_list;

/**
 * early_free_everything_to_pgalloc - free all early_alloc managed blocks to
 * pgalloc, which is assumed to be online now.
 */
void
early_free_everything_to_pgalloc (void)
{
	while (!free_list.empty ()) {
		early_free_block *block = free_list.pop_front ();
		uintptr_t addr = virt_to_phys ((uintptr_t) block);
		size_t size = block->size;

		size_t n = dsl::align_up (addr, PAGE_SIZE) - addr;
		if (n >= size)
			continue;

		addr += n;
		size = (size - n) / PAGE_SIZE;
		for (; size; addr += PAGE_SIZE, size--)
			free_page (phys_to_page (addr));
	}
}

/**
 * launder_phys_uintptr - begin the lifetime of a new object at addr.
 * @addr: physical address of storage location
 * @size: size of storage location
 * Returns addr.
 *
 * This function is needed to kill compiler aliasing analysis, which might
 * otherwise classify some things we do as UB.
 */
static uintptr_t
launder_phys_uintptr (uintptr_t addr, size_t size)
{
	return virt_to_phys (
		(uintptr_t) new (
			(void *) phys_to_virt (addr)
		) unsigned char[size]
	);
}

/**
 * early_alloc_phys_range - allocate physical memory.
 * @size: size, in bytes, to allocate
 * @align: required alignment in bytes
 * @low: the lowest possible address at which to allocate
 * @high: the highest possible address of which to allocate
 * Returns the physical address of an allocated memory block or zero on failure.
 *
 * The entire allocation has to fit within [low; high].
 */
uintptr_t
early_alloc_phys_range (size_t size, size_t align, uintptr_t low, uintptr_t high)
{
	low = dsl::align_up (dsl::max (low, minalign), minalign);
	high = dsl::align_down (high + 1, minalign) - 1;

	if (!low)
		return 0;

	size = dsl::align_up (size, minalign);
	if (!size)
		return 0;
	align = dsl::max (align, minalign);

	for (early_free_block *block: free_list) {
		uintptr_t start = virt_to_phys ((uintptr_t) block);
		uintptr_t end = start + block->size - 1;

		start = dsl::max (start, low);
		end = dsl::min (end, high);

		if (end < start)
			continue;

		uintptr_t start_aligned = dsl::align_up (start, align);
		if (end < start_aligned || start_aligned < start)
			continue;

		if (end - start_aligned + 1 < size)
			continue;

		if (start_aligned == start)
			block->linkage.remove ();
		else
			block->size = start_aligned - start;

		uintptr_t alloc_end = start_aligned + size;
		if (alloc_end != end) {
			early_free_block *block = (early_free_block *) phys_to_virt (alloc_end);
			block->size = end - alloc_end;
			free_list.push_front (block);
		}

		return launder_phys_uintptr (start_aligned, size);
	}

	return 0;
}

/**
 * early_free_phys - free physical memory.
 * @addr: address of physical memory block to free
 * @size: size of physical memory block to free
 */
void
early_free_phys (uintptr_t addr, size_t size)
{
	uintptr_t tmp = dsl::align_up (dsl::max (addr, minalign), minalign) - addr;
	if (tmp >= size)
		return;
	addr += tmp;
	size = dsl::align_down (size - tmp, minalign);
	if (!size)
		return;

	addr = launder_phys_uintptr (addr, size);
	early_free_block *block = (early_free_block *) phys_to_virt (addr);
	block->size = size;
	free_list.push_front (block);
}

/**
 * early_alloc_virt_zone - allocate virtual memory from a specific zone.
 * @size: size of memory to allocate
 * @align: required alignment of memory to allocate
 * @zonenr: the zone that the allocated memory has to belong to
 * Returns a pointer to the allocated memory in the kernel mapping.
 */
void *
early_alloc_virt_zone (size_t size, size_t align, int zonenr)
{
	uintptr_t low = zone_minaddr (zonenr);
	uintptr_t high = zone_maxaddr (zonenr);

	uintptr_t phys = early_alloc_phys_range (size, align, low, high);
	return phys ? (void *) phys_to_virt (phys) : nullptr;
}

/**
 * early_alloc_virt - allocate virtual memory.
 * @size: size of memory to allocate
 * @align: required alignment of memory to allocate
 */
void *
early_alloc_virt (size_t size, size_t align)
{
	int zonenr = allocation_zone (ALLOC_KERNEL);
	goto loop;

	while (zone_has_fallback (zonenr)) {
		zonenr = fallback_zone (zonenr);
loop:
		void *ptr = early_alloc_virt_zone (size, align, zonenr);
		if (ptr)
			return ptr;
	}

	return nullptr;
}

/**
 * early_free_virt - free virtual memory.
 * @ptr: pointer to virtual memory to free
 * @size: size of virtual memory to free
 */
void
early_free_virt (void *ptr, size_t size)
{
	early_free_phys (virt_to_phys ((uintptr_t) ptr), size);
}
