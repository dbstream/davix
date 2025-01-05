/**
 * Paging constants and page table layout.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_PAGE_DEFS_H
#define _ASM_PAGE_DEFS_H 1

#include <davix/stdint.h>

#define PAGE_SIZE _UL(0x1000)
#define P1D_SIZE _UL(0x200000)
#define P2D_SIZE _UL(0x40000000)

#define __PG_BIT(n) (_UL(1) << n)
#define __PG_PRESENT	__PG_BIT(0)
#define __PG_WRITE	__PG_BIT(1)
#define __PG_USER	__PG_BIT(2)
#define __PG_PWT	__PG_BIT(3)
#define __PG_PCD	__PG_BIT(4)
#define __PG_ACCESSED	__PG_BIT(5)
#define __PG_DIRTY	__PG_BIT(6)
#define __PG_PAT	__PG_BIT(7)
#define __PG_HUGE	__PG_BIT(7)
#define __PG_GLOBAL	__PG_BIT(8)
#define __PG_AVL1	__PG_BIT(9)
#define __PG_AVL2	__PG_BIT(10)
#define __PG_AVL3	__PG_BIT(11)
#define __PG_PAT_HUGE	__PG_BIT(12)
#define __PG_NX		__PG_BIT(63)

#define __PG_ADDR_MASK _UL(0x000ffffffffff000)

#ifndef __ASSEMBLER__

typedef unsigned long pfn_t;

typedef unsigned long pte_flags_t;

extern pte_flags_t x86_nx_bit;

/* Cache modes on x86. */
extern unsigned long PG_WT;
extern unsigned long PG_UC_MINUS;
extern unsigned long PG_UC;
extern unsigned long PG_WC;

#define PAGE_KERNEL_PGTABLE	(__PG_PRESENT | __PG_WRITE)
#define PAGE_USER_PGTABLE	(__PG_PRESENT | __PG_WRITE | __PG_USER)

#define PAGE_KERNEL_TEXT	(__PG_PRESENT | __PG_GLOBAL)
#define PAGE_KERNEL_RODATA	(__PG_PRESENT | __PG_GLOBAL | x86_nx_bit)
#define PAGE_KERNEL_DATA	(__PG_PRESENT | __PG_WRITE | __PG_GLOBAL | x86_nx_bit)

/**
 * Page table model
 *
 * There are max_pgtable_level page tables in the hierarchy, denoted as
 * p1d ... pNd (one-indexed). pgtable_shift(N) determines the amount to
 * right-shift an address to obtain the offset into the pNd of the page.
 * pgtable_entries(N) is the number of entries per pNd.
 */

extern int max_pgtable_level;

static inline int
pgtable_shift (int level)
{
	return 3 + (9 * level);
}

static inline int
pgtable_entries (int level)
{
	(void) level;
	return 512;
}

static inline unsigned long
pgtable_addr_index (unsigned long addr, int level)
{
	return (addr >> pgtable_shift (level)) & (pgtable_entries (level) - 1);
}

/* Start and end of range for which phys_to_virt/virt_to_phys is valid. */
extern unsigned long min_mapped_addr;
extern unsigned long max_mapped_addr;

/* Low and high range for vmap. */
extern unsigned long KERNEL_VMAP_LOW;
extern unsigned long KERNEL_VMAP_HIGH;

/** Start and end of the userspace mmap() range. */
extern unsigned long USER_MMAP_LOW;
extern unsigned long USER_MMAP_HIGH;

/**
 * Conversions between physical and virtual memory.
 */
extern unsigned long HHDM_OFFSET;

#define phys_to_virt(x) ((unsigned long) (x) + HHDM_OFFSET)
#define virt_to_phys(x) ((unsigned long) (x) - HHDM_OFFSET)

#endif /* !__ASSEMBLER__ */

#endif /* _ASM_PAGE_DEFS_H */
