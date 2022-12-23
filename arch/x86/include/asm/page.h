/* SPDX-License-Identifier: MIT */
#ifndef __ASM_PAGE_H
#define __ASM_PAGE_H

#include <davix/types.h>

typedef bitwise u64 pte_t;

typedef pte_t p1e_t;
typedef pte_t p2e_t;
typedef pte_t p3e_t;
typedef pte_t p4e_t;
typedef pte_t p5e_t;

typedef p1e_t *p1d_t;
typedef p2e_t *p2d_t;
typedef p3e_t *p3d_t;
typedef p4e_t *p4d_t;
typedef p5e_t *p5d_t;

/*
 * Architectural paging flags.
 */
#define __PG_PRESENT	((force pte_t) (1 << 0))
#define __PG_WRITE	((force pte_t) (1 << 1))
#define __PG_USER	((force pte_t) (1 << 2))
#define __PG_PWT	((force pte_t) (1 << 3))
#define __PG_PCD	((force pte_t) (1 << 4))
#define __PG_ACCESSED	((force pte_t) (1 << 5))
#define __PG_DIRTY	((force pte_t) (1 << 6))
#define __PG_SIZE	((force pte_t) (1 << 7))
#define __PG_GLOBAL	((force pte_t) (1 << 8))
#define __PG_NOEXEC	((force pte_t) (1UL << 63))

#define __P1E_PAT	((force p1e_t) (1 << 7))
#define __P2E_PAT	((force p2e_t) (1 << 12))
#define __P3E_PAT	((force p3e_t) (1 << 12))

#define PTE_ADDR(x)	(((force unsigned long) (x)) & 0xffffffffff000UL)

#define P1E_SHIFT 12
#define P2E_SHIFT 21
#define P3E_SHIFT 30
#define P4E_SHIFT 39
#define P5E_SHIFT 48

#define PAGE_SIZE (1UL << P1E_SHIFT)
#define P2E_SIZE (1UL << P2E_SHIFT)
#define P3E_SIZE (1UL << P3E_SHIFT)
#define P4E_SIZE (1UL << P4E_SHIFT)
#define P5E_SIZE (1UL << P5E_SHIFT)

#define P1E_MASK 0x1ff
#define P2E_MASK 0x1ff
#define P3E_MASK 0x1ff
#define P4E_MASK 0x1ff
#define P5E_MASK 0x1ff

#define P1E(x) (((x) >> P1E_SHIFT) & P1E_MASK)
#define P2E(x) (((x) >> P2E_SHIFT) & P2E_MASK)
#define P3E(x) (((x) >> P3E_SHIFT) & P3E_MASK)
#define P4E(x) (((x) >> P4E_SHIFT) & P4E_MASK)
#define P5E(x) (((x) >> P5E_SHIFT) & P5E_MASK)

extern unsigned long HHDM_OFFSET;

#define phys_to_virt(x) ((unsigned long) (x) + HHDM_OFFSET)
#define virt_to_phys(x) ((unsigned long) (x) - HHDM_OFFSET)

#endif /* __ASM_PAGE_H */
