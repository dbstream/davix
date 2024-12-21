/**
 * Memory management setup.
 * Copyright (C) 2024  dbstream
 */
#include <davix/align.h>
#include <davix/initmem.h>
#include <davix/page.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/stdbool.h>
#include <davix/string.h>
#include <asm/cregs.h>
#include <asm/early_memmap.h>
#include <asm/features.h>
#include <asm/mm_init.h>
#include <asm/msr.h>
#include <asm/pgtable.h>
#include <asm/sections.h>

pte_flags_t x86_nx_bit = 0;
int max_pgtable_level;

unsigned long HHDM_OFFSET;
unsigned long PFN_OFFSET;

static unsigned long HHDM_END, PFN_END;

unsigned long PG_WT = __PG_PWT;
unsigned long PG_UC_MINUS = __PG_PCD;
unsigned long PG_UC = __PG_PCD | __PG_PWT;
unsigned long PG_WC = __PG_PCD;

unsigned long min_mapped_addr = 0;
unsigned long max_mapped_addr = 0;

unsigned long KERNEL_VMAP_LOW;
unsigned long KERNEL_VMAP_HIGH;

__DATA_PAGE_ALIGNED __attribute__ ((aligned (PAGE_SIZE)))
unsigned long kernel_page_tables[512];

extern pgtable_t *
get_vmap_page_table (void)
{
	return (pgtable_t *) &kernel_page_tables[0];
}

/**
 * This defines an initial number of page tables which are allocated as part
 * of the kernel image. It needs to be large enough to map enough memory to
 * the point where initmem can allocate the rest we need.
 */
#define MAX_BOOT_PAGE_TABLES 16

__DATA_PAGE_ALIGNED __attribute__ ((aligned (PAGE_SIZE)))
static char boot_page_tables[PAGE_SIZE * MAX_BOOT_PAGE_TABLES];

static unsigned long
ktext_va (unsigned long p)
{
	return __KERNEL_START + p - kernel_load_offset;
}

static unsigned long
ktext_pa (void *p)
{
	return kernel_load_offset + (unsigned long) p - __KERNEL_START;
}

static unsigned long
pa (void *p)
{
	if ((unsigned long) p >= __KERNEL_START)
		return ktext_pa (p);
	return virt_to_phys (p);
}

__INIT_DATA static unsigned long boot_page_table_idx = 0;

__INIT_TEXT
static pgtable_t *
early_alloc_page_table (void)
{
	void *p = NULL;
	if (boot_page_table_idx < MAX_BOOT_PAGE_TABLES)
		p = &boot_page_tables[PAGE_SIZE * boot_page_table_idx++];

	if (!p)
		p = initmem_alloc_virt (PAGE_SIZE, PAGE_SIZE);

	if (p)
		memset (p, 0, PAGE_SIZE);

	return p;
}

/* During boot, use initmem. During normal runtime, use alloc_page. */

pgtable_t *
alloc_page_table (int level)
{
	(void) level;
	if (mm_is_early)
		return early_alloc_page_table ();

	struct pfn_entry *page = alloc_page (ALLOC_KERNEL | ALLOC_ZERO, 0);
	if (!page)
		return NULL;

	return (pgtable_t *) pfn_entry_to_virt (page);
}

void
free_page_table (int level, pgtable_t *table)
{
	(void) level;
	if (!table)
		return;

	initmem_free_virt (table, PAGE_SIZE);
}

__INIT_TEXT
void
x86_pgtable_init (void)
{
	/**
	 * memory layout with 48-bit virtual addresses:
	 *   0x0 .. 0x7fffffffffff			128 TiB	userspace
	 *   0xffff800000000000 .. 0xffffbfffffffffff	64 TiB	direct mappings of all physical memory
	 *   0xffffc00000000000 .. 0xffffc0ffffffffff	1 TiB	memory for struct pfn_entry
	 *   0xffffe00000000000 .. 0xffffefffffffffff	16 TiB	kernel vmap space
	 *   0xffffffff80000000 .. 0xffffffffffdfffff	~2 GiB	kernel and modules
	 *
	 * memory layout with 57-bit virtual addresses:
	 *   0x0 .. 0xffffffffffffff			64 PiB	userspace
	 *   0xff00000000000000 .. 0xff7fffffffffffff	32 PiB	direct mappings of all physical memory
	 *   0xff80000000000000 .. 0xff81ffffffffffff	512 TiB	memory for struct pfn_entry
	 *   0xff90000000000000 .. 0xffcfffffffffffff	16 PiB	kernel vmap space
	 *   0xffffffff80000000 .. 0xffffffffffdfffff	~2 GiB	kernel and modules
	 */

	if (bsp_has (FEATURE_LA57)) {
		max_pgtable_level = 5;
		HHDM_OFFSET = 0xff00000000000000UL;
		HHDM_END = 0xff7fffffffffffff;
		PFN_OFFSET = 0xff80000000000000UL;
		PFN_END = 0xff81ffffffffffffUL;
		KERNEL_VMAP_LOW = 0xff90000000000000UL;
		KERNEL_VMAP_HIGH = 0xffcfffffffffffffUL;
	} else {
		max_pgtable_level = 4;
		HHDM_OFFSET = 0xffff800000000000UL;
		HHDM_END = 0xffffbfffffffffffUL;
		PFN_OFFSET = 0xffffc00000000000UL;
		PFN_END = 0xffffc0ffffffffffUL;
		KERNEL_VMAP_LOW = 0xffffe00000000000UL;
		KERNEL_VMAP_HIGH = 0xffffefffffffffffUL;
	}

	if (bsp_has (FEATURE_NX)) {
		x86_nx_bit = __PG_NX;
		__efer_state |= _EFER_NXE;
		write_msr (MSR_EFER, __efer_state);
	}

	if (bsp_has (FEATURE_PAT)) {
		write_msr (MSR_PAT, 0x100070406UL);
		PG_WC = __PG_PAT;
	}
}

__INIT_DATA static unsigned long boot_pgt_start;
__INIT_DATA static unsigned long boot_pgt_end;

__INIT_TEXT
static pgtable_t *
get_pgtable_entry (unsigned long addr, int level)
{
	int current = max_pgtable_level;
	pgtable_t *pgtable = kernel_page_tables;
	pgtable += pgtable_addr_index (addr, current);
	while (current > level) {
		unsigned long entry = *pgtable;
		if (entry & __PG_PRESENT) {
			if (entry & __PG_HUGE)
				/**
				 * This memory is mapped at a higher level than
				 * our target level. Return NULL to indicate
				 * This.
				 */
				return NULL;
			entry &= __PG_ADDR_MASK;
			if (entry >= boot_pgt_start && entry < boot_pgt_end)
				pgtable = (pgtable_t *) ktext_va (entry);
			else
				pgtable = (pgtable_t *) phys_to_virt (entry);
		} else {
			pgtable_t *new_table = alloc_page_table (current - 1);
			if (!new_table)
				panic ("Cannot allocate kernel page table.");
			entry = pa (new_table) | PAGE_KERNEL_PGTABLE;
			*(volatile pgtable_t *) pgtable = entry;
			pgtable = new_table;
		}

		pgtable += pgtable_addr_index (addr, --current);
	}

	return pgtable;
}

__INIT_TEXT
static void
populate_page (unsigned long addr, unsigned long flags)
{
	pgtable_t *entry = get_pgtable_entry (addr, 1);
	if (!entry)
		panic ("populate_page: memory is hugepage-mapped");

	if (*entry)
		return;

	unsigned long mem = initmem_alloc_phys (PAGE_SIZE, PAGE_SIZE);
	if (!mem)
		panic ("populate_page: Out of memory");

	memset ((void *) phys_to_virt (mem), 0, PAGE_SIZE);
	*(volatile pgtable_t *) entry = mem | flags;
}

__INIT_TEXT
static void
set_page (unsigned long addr, unsigned long value, int level)
{
	volatile pgtable_t *entry =
		(volatile pgtable_t *) get_pgtable_entry (addr, level);

	if (!entry)
		/**
		 * get_pgtable_entry returns NULL if memory was mapped at a
		 * higher level than our target level.
		 */
		return;

	*entry = value;
}

__INIT_TEXT
static void
map_page (unsigned long addr, unsigned long value, int level)
{
	if (level > 1) {
		/* __PG_PAT is at a separate location for huge pages */
		if (value & __PG_PAT)
			value ^= __PG_PAT | __PG_PAT_HUGE;
		value |= __PG_HUGE;
	}

	set_page (addr, value, level);
}

__INIT_TEXT
static void
map_kernel_range (void *p_start, void *p_end, unsigned long flags)
{
	unsigned long start = ALIGN_DOWN((unsigned long) p_start, PAGE_SIZE);
	unsigned long end = ALIGN_UP((unsigned long) p_end, PAGE_SIZE);
	while (start < end) {
		map_page (start, ktext_pa ((void *) start) | flags, 1);
		start += PAGE_SIZE;
	}
}

__INIT_TEXT
static void
__map_hhdm_range (unsigned long start, unsigned long end, bool update_max)
{
	start = ALIGN_UP(start, PAGE_SIZE);
	end = ALIGN_DOWN(end, PAGE_SIZE);
	if (start >= end)
		return;

	unsigned long edge = min(unsigned long, end, ALIGN_UP(start, P1D_SIZE));
	while (start < edge) {
		map_page (phys_to_virt (start), start | PAGE_KERNEL_DATA, 1);
		start += PAGE_SIZE;
		if (update_max)
			max_mapped_addr = start;
	}

	if (!bsp_has (FEATURE_PDPE1GB))
		goto no_pdpe1gb;

	edge = min(unsigned long,
		ALIGN_DOWN(end, P1D_SIZE), ALIGN_UP(start, P2D_SIZE));
	while (start < edge) {
		map_page (phys_to_virt (start), start | PAGE_KERNEL_DATA, 2);
		start += P1D_SIZE;
		if (update_max)
			max_mapped_addr = start;
	}

	edge = ALIGN_DOWN(end, P2D_SIZE);
	while (start < edge) {
		map_page (phys_to_virt (start), start | PAGE_KERNEL_DATA, 3);
		start += P2D_SIZE;
		if (update_max)
			max_mapped_addr = start;
	}

no_pdpe1gb:
	edge = ALIGN_DOWN(end, P1D_SIZE);
	while (start < edge) {
		map_page (phys_to_virt (start), start | PAGE_KERNEL_DATA, 2);
		start += P1D_SIZE;
		if (update_max)
			max_mapped_addr = start;
	}

	while (start < end) {
		map_page (phys_to_virt (start), start | PAGE_KERNEL_DATA, 1);
		start += PAGE_SIZE;
		if (update_max)
			max_mapped_addr = start;
	}
}

__INIT_TEXT
void
map_hhdm_range (unsigned long start, unsigned long end)
{
	__map_hhdm_range (start, end, false);
}

/**
 * Map all entries in initmem_usable_ram.
 */
__INIT_TEXT
static void
map_hhdm (void)
{
	if (bsp_has (FEATURE_PDPE1GB))
		printk (PR_INFO "Using GB pages for direct mapping.\n");

	unsigned long start = 0, end = 0;
	for (unsigned long i = 0; i < initmem_usable_ram.num_entries; i++) {
		struct initmem_range range = initmem_usable_ram.entries[i];
		if (range.start != end) {
			if (start != end)
				__map_hhdm_range (start, end, true);
			start = range.start;
		}
		end = range.end;
	}

	if (start != end)
		__map_hhdm_range (start, end, true);
}

/**
 * Map all memory that we want mapped and switch over to the kernel page tables.
 * When this function returns, low-mapped memory becomes inaccessible.
 */
__INIT_TEXT
void
x86_paging_init (void)
{
	boot_pgt_start = ktext_pa (&boot_page_tables[0]);
	boot_pgt_end = ktext_pa (&boot_page_tables[
		PAGE_SIZE * MAX_BOOT_PAGE_TABLES]);

	map_kernel_range (__text_start, __text_end, PAGE_KERNEL_TEXT);
	map_kernel_range (__rodata_start, __rodata_end, PAGE_KERNEL_RODATA);
	map_kernel_range (__data_start, __data_end, PAGE_KERNEL_DATA);
	map_kernel_range (__cpulocal_virt_start, __cpulocal_virt_end, PAGE_KERNEL_DATA);
	map_kernel_range (__init_start, __init_end, PAGE_KERNEL_DATA & ~__PG_NX);

	/* allow early_memmap to work */
	set_page (0xffffffffffe00000UL,
		ktext_pa (__early_memmap_page) | PAGE_KERNEL_PGTABLE, 2);

	printk (PR_INFO "Switching to kernel mappings...\n");
	write_cr3 (ktext_pa (kernel_page_tables));

	/* Flush any global mappings from the TLB. */
	if (__cr4_state & __CR4_PGE)
		write_cr4 (__cr4_state & ~__CR4_PGE);
	__cr4_state |= __CR4_PGE;
	write_cr4 (__cr4_state);

	map_hhdm ();

	/**
	 * Disallow allocating from the static array, as it is internal to this
	 * file. External users applying virt_to_phys instead of pa (which is
	 * local to this file) would get incorrect results if alloc_page_table
	 * returned a page located in this array.
	 */
	boot_page_table_idx = MAX_BOOT_PAGE_TABLES;
}

__INIT_TEXT
void
arch_init_pfn_entry (void)
{
	printk ("x86/mm: mapping PFN database...\n");
	for (unsigned long i = 0; i < initmem_usable_ram.num_entries; i++) {
		struct initmem_range *range = &initmem_usable_ram.entries[i];
		unsigned long start = range->start, end = range->end;

		start = ALIGN_DOWN(start, PAGE_SIZE);
		end = ALIGN_UP(end, PAGE_SIZE);

		start = (unsigned long) phys_to_pfn_entry (start);
		end = (unsigned long) phys_to_pfn_entry (end);

		start = ALIGN_DOWN(start, PAGE_SIZE);
		end = ALIGN_UP(end, PAGE_SIZE);

		/* Populate the range [start, end). */
		for (; start < end; start += PAGE_SIZE)
			populate_page (start, PAGE_KERNEL_DATA);
	}
}

void
arch_insert_vmap_areas (void (*pfn_insert) (unsigned long, unsigned long, const char *))
{
	pfn_insert (HHDM_OFFSET, HHDM_END, "[higher-half direct map]");
	pfn_insert (PFN_OFFSET, PFN_END, "[struct pfn_entry]");
	pfn_insert ((unsigned long) __text_start, (unsigned long) __text_end - 1, "text(davix)");
	pfn_insert ((unsigned long) __rodata_start, (unsigned long) __rodata_end - 1, "rodata(davix)");
	pfn_insert ((unsigned long) __data_start, (unsigned long) __data_end - 1, "data(davix)");
	pfn_insert ((unsigned long) __cpulocal_virt_start, (unsigned long) __cpulocal_virt_end - 1, "cpulocal(davix)");
	pfn_insert ((unsigned long) __init_start, (unsigned long) __init_end - 1, "init(davix)");
}
