/* SPDX-License-Identifier: MIT */

#include <davix/list.h>
#include <davix/page_alloc.h>
#include <davix/printk.h>
#include <davix/kmalloc.h>
#include <davix/resource.h>
#include <davix/setup.h>
#include <asm/boot.h>
#include <asm/cpuid.h>
#include <asm/page.h>
#include <asm/segment.h>

unsigned long HHDM_OFFSET;
unsigned long PAGE_OFFSET;

unsigned long max_phys_addr;

int l5_paging_enable;
int l3_hugepage_enable;
pte_t x86_nx_bit;

static void init_vmem_constants(void)
{
	l5_paging_enable = x86_boot_struct.l5_paging_enable;

	max_phys_addr = 1UL << 32;
	x86_nx_bit = 0;
	l3_hugepage_enable = 0;

	HHDM_OFFSET = 0xffff800000000000UL;
	PAGE_OFFSET = 0xffffc00000000000UL;
	if(l5_paging_enable) {
		HHDM_OFFSET = 0xff00000000000000UL;
		PAGE_OFFSET = 0xff80000000000000UL;
	}

	unsigned long a, b, c, d, extmax;

	do_cpuid(0x80000000, a, b, c, d);
	extmax = a;

	if(extmax < 0x80000001)
		return;

	if(d & __ID_80000001_EDX_NX)
		x86_nx_bit = __PG_NOEXEC;
	if(d & __ID_80000001_EDX_PDPE1G)
		l3_hugepage_enable = 1;

	if(extmax < 0x80000008)
		return;

	do_cpuid(0x80000008, a, b, c, d);
	max_phys_addr = 1UL << min(l5_paging_enable ? 52 : 45, a & 0xff);
}

/*
 * Embedded into pages of memory to keep track of free pages.
 */
struct early_pagealloc_info {
	unsigned long count;
	struct list list;
};

static struct list early_pagealloc_list = LIST_INIT(early_pagealloc_list);

static void early_free_pages(unsigned long phys, unsigned long count)
{
	struct early_pagealloc_info *info =
		(struct early_pagealloc_info *) phys_to_virt(phys);
	info->count = count;
	list_add(&early_pagealloc_list, &info->list);
}

static unsigned long early_alloc_page(void)
{
	if(list_empty(&early_pagealloc_list))
		return 0;

	struct early_pagealloc_info *info =
		container_of(early_pagealloc_list.next,
			struct early_pagealloc_info, list);

	info->count--;
	unsigned long phys = virt_to_phys(info) + info->count * PAGE_SIZE;

	if(info->count == 0)
		list_del(&info->list);

	memset((void *) phys_to_virt(phys), 0, PAGE_SIZE);
	return phys;
}

static unsigned long pgtable_alloc(void)
{
	unsigned long ret = early_alloc_page();
	if(!ret) {
		critical("x86/mm: Couldn't allocate a page to hold page tables.\n");
		for(;;)
			relax();
	}
	return ret;
}

static void *kernel_pagetables;

static p4e_t *get_p4e(void *pgt, unsigned long vaddr)
{
	p5d_t p5d;
	p5e_t *p5e;
	p4d_t p4d;
	if(!l5_paging_enable) {
		p4d = (p4d_t) pgt;
		goto has_p4d;
	}

	p5d = (p5d_t) pgt;
	p5e = &p5d[P5E(vaddr)];
	if(!*p5e)
		*p5e = (force p5e_t) pgtable_alloc()
			| __PG_PRESENT | __PG_WRITE;
	p4d = (p4d_t) PTE_VADDR(*p5e);
has_p4d:
	return &p4d[P4E(vaddr)];
}

static p3e_t *get_p3e(void *pgt, unsigned long vaddr)
{
	p4e_t *p4e = get_p4e(pgt, vaddr);
	if(!*p4e)
		*p4e = (force p4e_t) pgtable_alloc()
			| __PG_PRESENT | __PG_WRITE;
	p3d_t p3d = (p3d_t) PTE_VADDR(*p4e);
	return &p3d[P3E(vaddr)];
}

static p2e_t *get_p2e(void *pgt, unsigned long vaddr)
{
	p3e_t *p3e = get_p3e(pgt, vaddr);
	if(!*p3e)
		*p3e = (force p3e_t) pgtable_alloc()
			| __PG_PRESENT | __PG_WRITE;
	p2d_t p2d = (p2d_t) PTE_VADDR(*p3e);
	return &p2d[P2E(vaddr)];
}

static p1e_t *get_p1e(void *pgt, unsigned long vaddr)
{
	p2e_t *p2e = get_p2e(pgt, vaddr);
	if(!*p2e)
		*p2e = (force p2e_t) pgtable_alloc()
			| __PG_PRESENT | __PG_WRITE;
	p1d_t p1d = (p1d_t) PTE_VADDR(*p2e);
	return &p1d[P1E(vaddr)];
}

static void map_mem_for_page_structs(unsigned long start, unsigned long end)
{
	pte_t flags = __PG_PRESENT | __PG_WRITE | __PG_GLOBAL | x86_nx_bit;

	start = (unsigned long) phys_to_page(align_down(start,
		PAGE_SIZE << (NUM_ORDERS - 1)));

	end = (unsigned long) phys_to_page(align_up(end,
		PAGE_SIZE << (NUM_ORDERS - 1)));

	for(; start < end; start += PAGE_SIZE) {
		p1e_t *p1e = get_p1e(kernel_pagetables, start);
		if(*p1e)
			continue;

		unsigned long mem = early_alloc_page();
		if(!mem) {
			critical("Couldn't allocate a page to hold page tables.\n");
			for(;;)
				relax();
		}
		*p1e = ((force p1e_t) mem) | flags;
	}
}

static struct resource system_memory;

void x86_setup_pat(void);	/* in arch/x86/mm/page_table.c */

void x86_setup_memory(void);
void x86_setup_memory(void)
{
	init_vmem_constants();
	if(l5_paging_enable)
		debug("x86/mm: Booted with five-level paging enabled.\n");

	struct memmap_entry *entry;
	info("x86/mm: Memory map:\n");
	list_for_each(entry, &x86_boot_struct.memmap_entries, list) {
		info("  [mem %p - %p] %s\n",
			entry->start, entry->end - 1,
			entry->type == MEMMAP_USABLE_RAM ? "System RAM" :
			entry->type == MEMMAP_LOADER ? "Loader" :
			entry->type == MEMMAP_ACPI_DATA ? "ACPI Tables" :
			entry->type == MEMMAP_KERNEL ? "Kernel" :
			entry->type == MEMMAP_RESERVED ? "Reserved" :
			entry->type == MEMMAP_ACPI_NVS ? "ACPI NVS Memory" :
			"Unknown memory type");

#if 0
		/*
		 * Useful when messing with page tables.
		 */
		if(entry->type == MEMMAP_USABLE_RAM) {
			unsigned long start = align_up(entry->start, PAGE_SIZE);
			unsigned long end = align_down(entry->end, PAGE_SIZE);
			for(; start < end; start += PAGE_SIZE) {
				*(volatile unsigned long *) phys_to_virt(start)
					= *(volatile unsigned long *)
						phys_to_virt(start) ^ start;
			}
		}
#endif
	}

	list_for_each(entry, &x86_boot_struct.memmap_entries, list) {
		if(entry->type != MEMMAP_USABLE_RAM)
			continue;
		unsigned long start = align_up(entry->start, PAGE_SIZE);
		unsigned long end = align_down(entry->end, PAGE_SIZE);
		early_free_pages(start, (end - start) / PAGE_SIZE);
	}

	kernel_pagetables = (void *) PTE_VADDR(read_cr3());
	list_for_each(entry, &x86_boot_struct.memmap_entries, list) {
		if(!(entry->type == MEMMAP_USABLE_RAM
			|| entry->type == MEMMAP_KERNEL
			|| entry->type == MEMMAP_LOADER
			|| entry->type == MEMMAP_ACPI_DATA))
				continue;
	
		map_mem_for_page_structs(entry->start, entry->end);
	}

	init_pagezones();

	struct early_pagealloc_info *info, *tmp;
	list_for_each_safe(info, &early_pagealloc_list, list, tmp) {
		unsigned long phys = virt_to_phys(info);
		unsigned long count = info->count;
		list_del(&info->list);
		while(count) {
			count--;
			free_page(phys_to_page(phys + count * PAGE_SIZE), 0);
		}
	}

	x86_setup_pat();

	init_kmalloc();

	if(init_resource(&system_memory,
		0, max_phys_addr, "System Memory", NULL)) {
			critical("init_resource() failed.\n");
			for(;;)
				relax();
	}

	struct resource *prev = NULL;
	list_for_each(entry, &x86_boot_struct.memmap_entries, list) {
		const char *name =
			entry->type == MEMMAP_USABLE_RAM ? "System RAM" :
			entry->type == MEMMAP_LOADER ? "System RAM" :
			entry->type == MEMMAP_ACPI_DATA ? "ACPI Tables" :
			entry->type == MEMMAP_KERNEL ? "Kernel" :
			entry->type == MEMMAP_RESERVED ? "Reserved" :
			entry->type == MEMMAP_ACPI_NVS ? "ACPI NVS Memory" :
			"Unknown";

		if(prev && prev->end == entry->start
			&& !strcmp(prev->name, name)) {
				prev->end = entry->end;
				continue;
		}
		if(!alloc_resource_at(&system_memory,
			entry->start, entry->end - entry->start, name)) {
				critical("alloc_resource_at() failed for [mem %p - %p].\n",
					entry->start,
					entry->end - 1);
				for(;;)
					relax();
		}
	}

	dump_resource(&system_memory);

	dump_pgalloc_info();
	dump_kmalloc_slabs();

	info("Copying the boot command line...\n");
	unsigned long cmdline_length = strlen(x86_boot_struct.cmdline) + 1;

	char *copied_cmdline = kmalloc(cmdline_length);
	if(!copied_cmdline) {
		critical("kmalloc() failed for %lu-character command line.\n",
			cmdline_length);
		for(;;)
			relax();
	}

	memcpy(copied_cmdline, x86_boot_struct.cmdline, cmdline_length);
	kernel_cmdline = copied_cmdline;
}
