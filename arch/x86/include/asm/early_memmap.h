/**
 * Early memory mapping capabilities.
 * Copyright (C) 2024  dbstream
 *
 * This is an interface which should only be used by the architecture at early
 * stages of boot, before the core mm is alive. It can map a very limited amount
 * of memory, so use with care.
 *
 * WARNING: none of the early_memmap interfaces provide any form of locking or
 * TLB invalidation on other CPUs than the one that is currently executing.
 */
#ifndef _ASM_EARLY_MEMMAP_H
#define _ASM_EARLY_MEMMAP_H 1

#include <asm/page_defs.h>

#define EARLY_MEMMAP_OFFSET		_UL(0xffffffffffe00000)

/**
 * WARNING!
 * Don't change the fixed entries if you are not absolutely sure what you
 * are doing!
 */
#define IDX_APIC_BASE			0
#define EARLY_MEMMAP_FIRST_DYN_IDX	1

extern unsigned long __early_memmap_page[];

/**
 * Store 'value' in __early_memmap_page[idx]. 'idx' must be < FIRST_DYN_IDX.
 *
 * 'value' is stored unmodified, so the caller must set the appropriate flags.
 The caller is also responsible for invalidating the TLB as necessary.
 */
extern void
early_memmap_fixed (unsigned long idx, unsigned long value);

extern void *
early_memmap (unsigned long phys, unsigned long size, pte_flags_t flags);

extern void
early_memunmap (void *virt);

#endif /* _ASM_EARLY_MEMMAP_H */
