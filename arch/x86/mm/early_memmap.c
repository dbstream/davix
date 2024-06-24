/**
 * Early memmap capability.
 * Copyright (C) 2024  dbstream
 *
 * At -2M we place a page table which is statically allocated in this file and
 * used by early_memmap and early_memunmap. However we make sure not to map the
 * absolute last page, in order to avoid processor bugs with overflow.
 */
#include <davix/printk.h>
#include <davix/stddef.h>
#include <davix/stdint.h>
#include <asm/early_memmap.h>
#include <asm/page_defs.h>
#include <asm/sections.h>
#include <asm/tlb.h>

#define NUM_SLOTS 511
_Alignas(PAGE_SIZE) unsigned long __early_memmap_page[NUM_SLOTS + 1];

__INIT_DATA static unsigned long slot_usage[NUM_SLOTS];

__INIT_TEXT
void
early_memmap_fixed (unsigned long idx, unsigned long value)
{
	*(volatile unsigned long *) &__early_memmap_page[idx] = value;
}

__INIT_TEXT
void *
early_memmap (unsigned long phys, unsigned long size, pte_flags_t flags)
{
	/* sanity checks */
	if (!phys || !size)
		return NULL;

	if (size > NUM_SLOTS * PAGE_SIZE)
		return NULL;

	unsigned long offset_in_page = phys & (PAGE_SIZE - 1);
	pfn_t pfn = phys / PAGE_SIZE;

	size += offset_in_page;
	unsigned long num_slots = (size + PAGE_SIZE - 1) / PAGE_SIZE;

	/* Find enough contiguous free slots. */
	unsigned long a = EARLY_MEMMAP_FIRST_DYN_IDX;
	unsigned long b = a;
	while (b < a + num_slots) {
		if (b >= NUM_SLOTS) {
			printk (PR_WARN "early_memmap: too few slots for %#lx-sized allocation\n",
				size);
			return NULL;
		}

		if (slot_usage[b]) {
			a = b + slot_usage[b];
			b = a;
		} else {
			b++;
		}
	}

	/* The range [a, b) is free and can be used for the mapping. */
	slot_usage[a] = num_slots;
	unsigned long virt = EARLY_MEMMAP_OFFSET + a * PAGE_SIZE;
	for (; a < b; a++, pfn++) {
		volatile unsigned long *entry = &__early_memmap_page[a];
		*entry = (pfn * PAGE_SIZE) | flags;
	}

	return (void *) (virt + offset_in_page);
}

__INIT_TEXT
void
early_memunmap (void *virt)
{
	unsigned long offset = (unsigned long) virt - EARLY_MEMMAP_OFFSET;
	unsigned long slot = offset / PAGE_SIZE;
	unsigned long num_slots = slot_usage[slot];

	slot_usage[slot] = 0;
	while (num_slots) {
		volatile unsigned long *entry = &__early_memmap_page[slot];
		*entry = 0;
		__invlpg ((void *) (EARLY_MEMMAP_OFFSET + slot * PAGE_SIZE));

		slot++;
		num_slots--;
	}
}
