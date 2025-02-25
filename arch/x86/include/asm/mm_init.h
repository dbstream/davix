/**
 * Memory management setup.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_MM_INIT_H
#define _ASM_MM_INIT_H 1

/* Physical memory offset at which the kernel is loaded. */
extern unsigned long kernel_load_offset;

/* Kernel page tables. */
extern unsigned long kernel_page_tables[];

/* Very early initialization of paging. */
extern void
x86_pgtable_init (void);

/* Map all memory, switch to kernel page tables, etc... */
extern void
x86_paging_init (void);

/* Map physical memory between start and end. */
extern void
map_hhdm_range (unsigned long start, unsigned long end);

/* Insert implicit areas into vmap. */
extern void
arch_insert_vmap_areas (void (*pfn_insert) (unsigned long, unsigned long, const char *));

void
x86_mm_init_late (void);

#endif /* _ASM_MM_INIT_H */
