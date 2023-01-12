/* SPDX-License-Identifier: MIT */
#include <davix/errno.h>
#include <davix/mm.h>
#include <davix/page_alloc.h>
#include <davix/page_table.h>
#include <davix/printk.h>
#include <davix/list.h>
#include <asm/cpuid.h>
#include <asm/segment.h>
#include <asm/msr.h>
#include <asm/page.h>
#include <asm/tlb.h>

static int use_pat;
static unsigned long msrpat_value;

static pte_t pgcachemode_to_pteflags_table[NUM_PG_CACHEMODES] = {
	[PG_WRITEBACK] = 0,
	[PG_WRITETHROUGH] =                         __PG_PWT,
	[PG_WRITECOMBINE] =   __P1E_PAT,
	[PG_UNCACHED_MINUS] =            __PG_PCD,
	[PG_UNCACHED] =	                 __PG_PCD | __PG_PWT
};

static inline pte_t pgcachemode_to_pteflags(pgcachemode_t cachemode, struct pgop *pgop)
{
	pte_t pteflags = pgcachemode_to_pteflags_table[cachemode];
	if((pteflags & __P1E_PAT) && pgop->shift != 0) {
		pteflags &= ~__P1E_PAT;
		pteflags |= __P2E_PAT;
	}
	return pteflags;
}

static_assert(__P2E_PAT == __P3E_PAT, "page flags");

void x86_setup_pat(void);
void x86_setup_pat(void)
{
	unsigned long a, b, c, d;
	do_cpuid(1, a, b, c, d);

	if(!(d & __ID_00000001_EDX_PAT)) {
		warn("x86/mm: no PAT mechanism, all write-combine mappings will be uncached.\n");
		use_pat = 0;
		pgcachemode_to_pteflags_table[PG_WRITECOMBINE] =
			pgcachemode_to_pteflags_table[PG_UNCACHED];
		return;
	}

	use_pat = 1;
	msrpat_value = 0x100070406UL; /* UC  UC  UC  WC  UC  UC-  WT  WB */

	debug("x86/mm: enable PAT mechanism.\n");
	write_msr(MSR_PAT, msrpat_value);
}

static void refcount_page_tables(pte_t *page_tables, unsigned level)
{
	struct page *page = virt_to_page(page_tables);
	page->pte.refcnt = 0;
	for(unsigned i = 0; i < 512; i++) {
		pte_t entry = page_tables[i];
		if(!(entry & __PG_PRESENT))
			continue;
		page->pte.refcnt++;
		if(level == 1 || (entry & __PG_SIZE))
			continue;
		refcount_page_tables((pte_t *) PTE_VADDR(entry), level - 1);
	}
}

#define ALLOC_PGTABLE (ALLOC_KERNEL | __ALLOC_ZERO)

static struct page *new_pgtable(void)
{
	struct page *page = alloc_page(ALLOC_PGTABLE, 0);
	if(!page)
		return NULL;

	page->pte.refcnt = 0;
	return page;
}

void x86_setup_nonearly_page_tables(void);
void x86_setup_nonearly_page_tables(void)
{
	debug("x86/mm: Setup non-early page tables.\n");
	pte_t *page_tables = (pte_t *) PTE_VADDR(read_cr3());
	kernelmode_mm.page_tables = page_tables;

	for(unsigned i = 0; i < 256; i++)
		page_tables[i] = 0;

	for(unsigned i = 256; i < 512; i++) {
		if(page_tables[i])
			continue;
		struct page *page = new_pgtable();
		page_tables[i] = __PG_PRESENT | __PG_WRITE | __PG_GLOBAL
			| ((force pte_t) page_to_phys(page));
	}
	refcount_page_tables(page_tables, l5_paging_enable ? 5 : 4);
}

static void pgop_flush_range(struct pgop *pgop,
	unsigned long start, unsigned long end)
{
	if(pgop->flush_start == pgop->flush_end) {
		pgop->flush_start = start;
		pgop->flush_end = end;
		return;
	}

	unsigned shift = 12 + pgop->shift;
	if(!((pgop->flush_start >> shift) > (end >> shift) + 512
		|| (pgop->flush_end >> shift) + 512 < (start >> shift)))
			return;

	tlb_flush(pgop);
	pgop->flush_start = start;
	pgop->flush_end = end;
}

static void pgop_flush(struct pgop *pgop, unsigned long addr)
{
	pgop_flush_range(pgop, addr, addr + (PAGE_SIZE << pgop->shift));
}

static void pgop_free_page(struct pgop *pgop, struct page *page)
{
	list_radd(&pgop->delayed_pages, &page->pte.delayed_free_list);
}

int has_hugepage_size(unsigned shift)
{
	if(shift == 0 || shift == 9)
		return 1;

	if(shift == 18 && l3_hugepage_enable)
		return 1;

	return 0;
}

void pgop_begin(struct pgop *pgop, struct mm *mm, unsigned shift)
{
	pgop->flush_start = 0;
	pgop->flush_end = 0;
	list_init(&pgop->delayed_pages);
	pgop->mm = mm;
	pgop->shift = shift;
}

void pgop_end(struct pgop *pgop)
{
	tlb_flush(pgop);
}

static void ref_pgtable(pte_t *pgtable)
{
	struct page *containing_page = virt_to_page(pgtable);
	containing_page->pte.refcnt++;
}

static int deref_pgtable(pte_t *pgtable)
{
	struct page *containing_page = virt_to_page(pgtable);
	containing_page->pte.refcnt--;
	return containing_page->pte.refcnt == 0;
}

static int pgtable_referenced(pte_t *pgtable)
{
	struct page *containing_page = virt_to_page(pgtable);
	return containing_page->pte.refcnt != 0;
}

static int alloc_single_pte(struct pgop *pgop, unsigned long addr)
{
	pte_t pgtable_flags = __PG_PRESENT | __PG_WRITE;
	if(pgop->mm == &kernelmode_mm) {
		pgtable_flags |= __PG_GLOBAL;
	} else {
		pgtable_flags |= __PG_USER;
	}

	/* Give these a value. (-Werror=maybe-uninitialized) */
	p5d_t p5d = NULL;
	p5e_t *p5e = NULL;

	p4d_t p4d;
	if(l5_paging_enable) {
		p5d = (p5d_t) pgop->mm->page_tables;
		p5e = &p5d[P5E(addr)];
		if(!*p5e) {
			struct page *page = new_pgtable();
			if(!page)
				return ENOMEM;

			*p5e = ((force p5e_t) page_to_phys(page))
				| pgtable_flags;
			ref_pgtable(p5d);
		}

		p4d = (p4d_t) PTE_VADDR(*p5e);
	} else {
		p4d = (p4d_t) pgop->mm->page_tables;
	}

	p4e_t *p4e = &p4d[P4E(addr)];
	if(!*p4e) {
		struct page *page = new_pgtable();
		if(!page)
			goto no_p3d;

		*p4e = ((force p4e_t) page_to_phys(page)) | pgtable_flags;
		ref_pgtable(p4d);
	}

	p3d_t p3d = (p3d_t) PTE_VADDR(*p4e);
	if(pgop->shift == 18) {
		ref_pgtable(p3d);
		return 0;
	}

	p3e_t *p3e = &p3d[P3E(addr)];
	if(!*p3e) {
		struct page *page = new_pgtable();
		if(!page)
			goto no_p2d;

		*p3e = ((force p3e_t) page_to_phys(page)) | pgtable_flags;
		ref_pgtable(p3d);
	}

	p2d_t p2d = (p2d_t) PTE_VADDR(*p3e);
	if(pgop->shift == 9) {
		ref_pgtable(p2d);
		return 0;
	}

	p2e_t *p2e = &p2d[P2E(addr)];
	if(!*p2e) {
		struct page *page = new_pgtable();
		if(!page)
			goto no_p1d;

		*p2e = ((force p2e_t) page_to_phys(page)) | pgtable_flags;
		ref_pgtable(p2d);
	}

	p1d_t p1d = (p1d_t) PTE_VADDR(*p2e);
	ref_pgtable(p1d);
	return 0;
no_p1d:
	if(!pgtable_referenced(p2d)) {
		pgop_free_page(pgop, virt_to_page(p2d));
		*p3e = 0;
		deref_pgtable(p3d);
	}

no_p2d:
	if(!pgtable_referenced(p3d)) {
		pgop_free_page(pgop, virt_to_page(p3d));
		*p4e = 0;
		deref_pgtable(p4d);
	}

no_p3d:
	if(!l5_paging_enable)
		return ENOMEM;

	if(!pgtable_referenced(p4d)) {
		pgop_free_page(pgop, virt_to_page(p4d));
		*p5e = 0;
		deref_pgtable(p5d);
	}

	return ENOMEM;
}

static void free_single_pte(struct pgop *pgop, unsigned long addr)
{
	p5d_t p5d;
	p5e_t *p5e;
	p4d_t p4d;
	if(l5_paging_enable) {
		p5d = (p5d_t) pgop->mm->page_tables;
		p5e = &p5d[P5E(addr)];
		p4d = (p4d_t) PTE_VADDR(*p5e);
	} else {
		p4d = (p4d_t) pgop->mm->page_tables;
	}

	p4e_t *p4e = &p4d[P4E(addr)];
	p3d_t p3d = (p3d_t) PTE_VADDR(*p4e);
	if(pgop->shift == 18)
		goto deref_p3d;

	p3e_t *p3e = &p3d[P3E(addr)];
	p2d_t p2d = (p2d_t) PTE_VADDR(*p3e);
	if(pgop->shift == 9)
		goto deref_p2d;

	p2e_t *p2e = &p2d[P2E(addr)];
	p1d_t p1d = (p1d_t) PTE_VADDR(*p2e);

	if(!deref_pgtable(p1d))
		return;
	pgop_free_page(pgop, virt_to_page(p1d));
	*p2e = 0;
deref_p2d:
	if(!deref_pgtable(p2d))
		return;
	pgop_free_page(pgop, virt_to_page(p2d));
	*p3e = 0;
deref_p3d:
	if(!deref_pgtable(p3d))
		return;
	pgop_free_page(pgop, virt_to_page(p3d));
	*p4e = 0;

	if(!l5_paging_enable)
		return;

	if(!deref_pgtable(p4d))
		return;
	pgop_free_page(pgop, virt_to_page(p4d));
	*p5e = 0;
}

int alloc_ptes(struct pgop *pgop,
	unsigned long start_addr, unsigned long end_addr)
{
	unsigned long skip = (PAGE_SIZE << pgop->shift);
	for(unsigned long i = start_addr; i < end_addr; i += skip) {
		int err = alloc_single_pte(pgop, i);
		if(!err)
			continue;
		for(unsigned long j = start_addr; j < i; j += skip)
			free_single_pte(pgop, j);
		return err;
	}
	return 0;
}

void free_ptes(struct pgop *pgop,
	unsigned long start_addr, unsigned long end_addr)
{
	unsigned long skip = (PAGE_SIZE << pgop->shift);
	for(unsigned long i = start_addr; i < end_addr; i += skip)
		free_single_pte(pgop, i);
}

static int pte_needs_flush(pte_t oldval, pte_t newval)
{
	if(!(oldval & __PG_PRESENT))
		return 0;

	if((oldval & __PG_PRESENT) && !(newval & __PG_PRESENT))
		return 1;

	if((oldval & __PG_WRITE) && !(newval & __PG_WRITE))
		return 1;

	if((newval & __PG_NOEXEC) && !(oldval & __PG_NOEXEC))
		return 1;

	return 0;
}

static void update_pte(struct pgop *pgop,
	unsigned long addr, pte_t *pte, pte_t newval)
{
	pte_t oldval = *pte;
	*pte = newval;
	if(pte_needs_flush(oldval, newval))
		pgop_flush(pgop, addr);
}

void set_pte(struct pgop *pgop, unsigned long addr, unsigned long phys_addr,
	pgcachemode_t cachemode, pgmode_t mode)
{
	pte_t pteval;
	if(mode == 0) {
		pteval = 0;
		goto has_pteval;
	}

	pteval = ((force pte_t) phys_addr)
		| pgcachemode_to_pteflags(cachemode, pgop)
		| __PG_PRESENT;

	if(pgop->shift != 0)
		pteval |= __PG_SIZE;

	if(pgop->mm == &kernelmode_mm) {
		pteval |= __PG_GLOBAL;
	} else {
		pteval |= __PG_USER;
	}

	if(mode & PG_WRITABLE)
		pteval |= __PG_WRITE;

	if(!(mode & PG_EXECUTABLE))
		pteval |= x86_nx_bit;

has_pteval:;
	p4d_t p4d;
	if(l5_paging_enable) {
		p5d_t p5d = (p5d_t) pgop->mm->page_tables;
		p4d = (p4d_t) PTE_VADDR(p5d[P5E(addr)]);
	} else {
		p4d = (p4d_t) pgop->mm->page_tables;
	}
	p3d_t p3d = (p3d_t) PTE_VADDR(p4d[P4E(addr)]);
	if(pgop->shift == 18) {
		update_pte(pgop, addr, &p3d[P3E(addr)], pteval);
		return;
	}
	p2d_t p2d = (p2d_t) PTE_VADDR(p3d[P3E(addr)]);
	if(pgop->shift == 9) {
		update_pte(pgop, addr, &p2d[P2E(addr)], pteval);
		return;
	}
	p1d_t p1d = (p1d_t) PTE_VADDR(p2d[P2E(addr)]);
	update_pte(pgop, addr, &p1d[P1E(addr)], pteval);
}
