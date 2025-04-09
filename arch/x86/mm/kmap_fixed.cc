/**
 * kmap_fixed:  allocation-free memory mappings that can be used during boot.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/asm.h>
#include <asm/kmap_fixed.h>

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
