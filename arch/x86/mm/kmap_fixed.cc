/**
 * kmap_fixed:  allocation-free memory mappings that can be used during boot.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/asm.h>
#include <asm/kmap_fixed.h>
#include <dsl/align.h>

extern "C" { volatile uint64_t __kmap_fixed_page alignas (PAGE_SIZE) [512]; }

void
kmap_fixed_clear (int idx)
{
	__kmap_fixed_page[idx] = make_empty_pte ().value;
	__invlpg (kmap_fixed_address (idx));
}

void *
kmap_fixed_install (int idx, pte_t pte)
{
	uint64_t old_value = __kmap_fixed_page[idx];
	__kmap_fixed_page[idx] = pte.value;
	if (old_value != 0)
		__invlpg (kmap_fixed_address (idx));
	return (void *) kmap_fixed_address (idx);
}

static int inuse[512];

int
kmap_fixed_alloc_indices (int num)
{
	if (num <= 0 || num > 511 - KMAP_FIXED_IDX_FIRST_DYNAMIC)
		return 0;

	int a = KMAP_FIXED_IDX_FIRST_DYNAMIC;
	int b = a;

	while (b < a + num) {
		if (b > 511)
			return 0;
		if (inuse[b]) {
			a = b + inuse[b];
			b = a;
		} else
			b++;
	}

	inuse[a] = num;
	return a;
}

void
kmap_fixed_free_indices (int idx)
{
	int n = inuse[idx];
	inuse[idx] = 0;
	for (int i = 0; i < n; i++)
		kmap_fixed_clear (idx + i);
}

void *
kmap_fixed (uintptr_t phys, size_t size, pteval_t flags)
{
	if (size > (511 - KMAP_FIXED_IDX_FIRST_DYNAMIC) * PAGE_SIZE)
		return nullptr;

	uintptr_t offset = phys & (PAGE_SIZE - 1);
	phys -= offset;
	size += offset;
	size = dsl::align_up (size, PAGE_SIZE);
	int npages = size / PAGE_SIZE;

	int idx = kmap_fixed_alloc_indices (npages);
	if (!idx)
		return nullptr;

	for (int i = 0; i < npages; i++)
		kmap_fixed_install (idx + i,
				__make_pte (phys + i * PAGE_SIZE, flags));

	return (void *) (kmap_fixed_address (idx) + offset);
}

void
kunmap_fixed (void *ptr)
{
	int idx = ((uintptr_t) ptr - KMAP_FIXED_BASE) / PAGE_SIZE;
	kmap_fixed_free_indices (idx);
}
