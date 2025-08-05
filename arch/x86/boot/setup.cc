// SPDX-License-Identifier: GPL-3.0
/*
 * File: boot/setup.cc
 * HalStartKernel() and early architecture-specific initialization.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/mm.h>
#include <Hal/page_tables.h>
#include <Hal/percpu.h>
#include <Ke/console.h>
#include <Ke/log.h>
#include <Ki/start_kernel.h>
#include <Mm/page_alloc.h>
#include <Mm/pfn.h>
#include <asm/cpufeature.h>
#include <asm/creg_access.h>
#include <asm/creg_bits.h>
#include <asm/idt.h>
#include <asm/io.h>
#include <string.h>
#include "multiboot.h"
#include "../Hal/internal.h"

static multiboot_params *boot_params;
static unsigned long load_offset;

static unsigned long ktext_pa(void *ptr)
{
	return load_offset + ((unsigned long) ptr) - 0xffffffff80000000UL;
}

static multiboot_tag *__mb2_next(multiboot_tag *t)
{
	unsigned long x = (unsigned long) t;
	return (multiboot_tag *) ((x + t->size + 7UL) & ~7UL);
}

constexpr static struct {
	struct iterator {
		multiboot_tag *m_ptr;

		iterator (multiboot_tag *ptr)
			: m_ptr (ptr)
		{}

		multiboot_tag *
		operator* (void) const
		{
			return m_ptr;
		}

		bool
		operator!= (const iterator &other) const
		{
			return m_ptr != other.m_ptr;
		}

		iterator &
		operator++ (void)
		{
			m_ptr = __mb2_next(m_ptr);
			if (m_ptr->type == MB2_TAG_END)
				m_ptr = nullptr;
			return *this;
		}
	};

	iterator
	begin (void) const
	{
		return { (multiboot_tag *) ((uintptr_t) boot_params + 8) };
	}

	iterator
	end (void) const
	{
		return iterator (nullptr);
	}
} mb2_tags;

static unsigned long max_supported_ram;

static multiboot_memmap *mb2_memmap = nullptr;
static int memmap_len = 0;
static unsigned long memmap_entry_size;

static inline void *memmap_entry_ptr(int idx)
{
	return (void *) ((uintptr_t) mb2_memmap + 16 + memmap_entry_size * idx);
}

static inline multiboot_memmap_entry memmap_entry(int idx)
{
	return *(multiboot_memmap_entry *) memmap_entry_ptr(idx);
}

static constexpr int max_blockers = 4;
static int num_blockers = 0;
static unsigned long block_start[max_blockers], block_end[max_blockers];

static void block_memory(unsigned long start, unsigned long end)
{
	if (num_blockers >= max_blockers)
		KePanic("HalStartKernel: too many reserved memory regions");

	block_start[num_blockers] = start;
	block_end[num_blockers] = end;
	num_blockers++;
}

static int alloc_index;
static unsigned long alloc_wmark;

/**
 * free_memory_to_page_allocator - free physical pages to the page allocator
 * @start: start of physical memory region to free
 * @end: end of physical memory region to free
 */
static void free_memory_to_page_allocator(unsigned long start, unsigned long end)
{
	start = PGALIGN_UP(start);
	end = PGALIGN_DOWN(end);
	KePrintf("Freeing memory range [0x%tx - 0x%tx]\n", start, end);
	while (start < end) {
		MMPFN *pfn = MmGetPFNForPhys(start);
		MmFreePage(pfn);
		start += PAGE_SIZE;
	}
}

/**
 * init_free_memory - free all unallocated usable RAM to the page allocator.
 */
static void init_free_memory(void)
{
	for (int i = 0; i < memmap_len; i++) {
		multiboot_memmap_entry entry = memmap_entry(i);
		if (entry.type != MB2_MEMMAP_USABLE)
			continue;

		unsigned long start = entry.start;
		unsigned long end = start + entry.size;
		if (alloc_wmark < end)
			end = alloc_wmark;

		bool was_blocked;
		do {
			if (end <= start)
				break;
			was_blocked = false;
			unsigned long bs = end, be = end;
			for (int j = 0; j < num_blockers; j++) {
				if (block_end[j] <= start)
					continue;
				if (block_start[j] >= end)
					continue;
				if (block_start[j] < bs) {
					bs = block_start[j];
					be = block_end[j];
					was_blocked = true;
				}
			}
			if (start < bs)
				free_memory_to_page_allocator(start, bs);
			start = be;
		} while (was_blocked);
	}
}

static unsigned long alloc_from_memmap(unsigned long size, unsigned long align)
{
	/*
	 * We require @size and @align to be power-of-two aligned and to be at
	 * minimum the page size.  Additionally, we require @align to be smaller
	 * than or equal to @size.
	 */
	BUG_ON(size < PAGE_SIZE);
	BUG_ON(size & (size - 1UL));
	BUG_ON(align < PAGE_SIZE);
	BUG_ON(align & (align - 1UL));
	BUG_ON(size < align);

	for (;;) {
		multiboot_memmap_entry entry = memmap_entry(alloc_index);
		if (entry.type != MB2_MEMMAP_USABLE) {
			if (!alloc_index)
				KePanic("Out of memory in early boot!");
			alloc_index--;
			continue;
		}

		unsigned long start = entry.start;
		unsigned long end = start + entry.size;

		if (alloc_wmark > end)
			alloc_wmark = end;

		bool was_blocked;
		do {
			was_blocked = false;
			alloc_wmark &= ~(align - 1UL);
			if (alloc_wmark < size)
				KePanic("Out of memory in early boot!");
			for (int i = 0; i < num_blockers; i++) {
				if (block_start[i] >= alloc_wmark)
					continue;
				if (block_end[i] <= alloc_wmark - size)
					continue;
				alloc_wmark = block_start[i];
				was_blocked = true;
				break;
			}
		} while (was_blocked);

		if (alloc_wmark - size < start) {
			if (!alloc_index)
				KePanic("Out of memory in early boot!");
			alloc_index--;
			continue;
		}

		alloc_wmark -= size;
		return alloc_wmark;
	}
}

static unsigned long alloc_page_from_memmap(void)
{
	return alloc_from_memmap(PAGE_SIZE, PAGE_SIZE);
}

extern "C" char __kernel_start[];
extern "C" char __kernel_end[];
extern "C" char __head_start[];
extern "C" char __head_end[];
extern "C" char __text_start[];
extern "C" char __text_end[];
extern "C" char __rodata_start[];
extern "C" char __rodata_end[];
extern "C" char __data_start[];
extern "C" char __data_end[];
extern "C" char __percpu_start[];
extern "C" char __percpu_end[];

static unsigned long total_ram_bytes = 0;

static void map_hhdm_range_pa(unsigned long start_pa, unsigned long end_pa)
{
	unsigned long start_va = MiPhysToVirt(start_pa);
	unsigned long end_va = MiPhysToVirt(end_pa);

	KePrintf("Mapping HHDM range [0x%tx - 0x%tx]\n", start_va, end_va);
	HalMapRangeHHDM(start_va, end_va);
}

static void map_pfn_range(unsigned long start, unsigned long end)
{
	KePrintf("Mapping MMPFN range [0x%tx - 0x%tx]\n", start, end);
	HalMapRangePFN(start, end);
}

static void map_kernel(MMPTEP ptes, char *pstart , char *pend, PTEFLAGS flags)
{
	unsigned long phy = ktext_pa(pstart);
	unsigned long start = pstart - __kernel_start;
	unsigned long end = pend - __kernel_start;

	start = PGALIGN_DOWN(start);
	end = PGALIGN_UP(end);

	ptes += start / PAGE_SIZE;
	while (start < end) {
		HalWritePTE(ptes, HalMakeKPTE(1, phy, flags));
		ptes++;
		phy += PAGE_SIZE;
		start += PAGE_SIZE;
	}
}

static void init_memory(void)
{
	block_memory(0, PAGE_SIZE); // Reserve the lowest page ("zero'th page").
	block_memory(ktext_pa(__kernel_start), ktext_pa(__kernel_end));
	block_memory((unsigned long) boot_params,
			(unsigned long) boot_params + boot_params->size);

	for (multiboot_tag *tag : mb2_tags) {
		if (tag->type == MB2_TAG_MEMMAP) {
			mb2_memmap = (multiboot_memmap *) tag;
			break;
		}
	}

	if (!mb2_memmap)
		KePanic("Bootloader provided no memory map!");

	memmap_entry_size = mb2_memmap->entry_size;
	if (memmap_entry_size < sizeof(multiboot_memmap_entry))
		KePanic("Bootloader provided an invalid memory map!");

	memmap_len = (mb2_memmap->size - 16) / memmap_entry_size;
	if (!memmap_len)
		KePanic("Bootloader provided an invalid memory map!");

	alloc_index = memmap_len - 1;
	alloc_wmark = max_supported_ram;

	KePrintf("Memory map:\n");
	for (int i = 0; i < memmap_len; i++) {
		multiboot_memmap_entry entry = memmap_entry(i);

		unsigned long start = entry.start;
		unsigned long end = start + entry.size - 1UL;
		unsigned int type = entry.type;
		KePrintf("  [%2d] [0x%tx - 0x%tx] ", i, start, end);

		switch(type) {
		case MB2_MEMMAP_USABLE:
			KePrintf("Usable RAM\n");
			total_ram_bytes += entry.size;
			break;
		case 2:
			KePrintf("Reserved\n");
			break;
		case MB2_MEMMAP_ACPI_RECLAIM:
			KePrintf("ACPI Reclaimable\n");
			total_ram_bytes += entry.size;
			break;
		case MB2_MEMMAP_ACPI_NVS:
			KePrintf("ACPI NVS Memory\n");
			break;
		case MB2_MEMMAP_DEFECTIVE:
			KePrintf("Defective RAM\n");
			break;
		default:
			KePrintf("type %u\n", type);
			break;
		}
	}

	KePrintf("Total RAM bytes: %lu  (%lu MiB)\n",
			total_ram_bytes,
			total_ram_bytes / 1048576);

	HalSetEarlyPageAllocationFunction(alloc_page_from_memmap);

	unsigned long map_start = 0, map_end = 0;
	for (int i = 0; i < memmap_len; i++) {
		multiboot_memmap_entry entry = memmap_entry(i);
		unsigned long start = entry.start;
		unsigned long end = start + entry.size;
		unsigned int type = entry.type;

		if (type != MB2_MEMMAP_USABLE && type != MB2_MEMMAP_ACPI_RECLAIM)
			continue;

		if (map_end == start) {
			map_end = end;
			continue;
		}

		if (map_start != map_end)
			map_hhdm_range_pa(map_start, map_end);

		map_start = start;
		map_end = end;
	}
	if (map_start != map_end)
		map_hhdm_range_pa(map_start, map_end);
	/*
	 * Setup memory mappings for the kernel text itself: we have to be
	 * careful, because we are changing virtual address space that we
	 * currently live in.  First construct the new kernel PML1(s), then
	 * atomically replace a PML2e with a PML1 containing the same mappings.
	 */
	unsigned long kernel_bytes = __kernel_end - __kernel_start;
	unsigned long num_kernel_ptes = PGALIGN_UP(kernel_bytes) / PAGE_SIZE;
	unsigned long num_kernel_pgtables = (num_kernel_ptes + 511) / 512;

	MMPTEP kptes = (MMPTEP) MiPhysToVirt(alloc_from_memmap(
			PAGE_SIZE * num_kernel_pgtables, PAGE_SIZE
	));

	bzero(kptes, PAGE_SIZE * num_kernel_pgtables);

	map_kernel(kptes, __head_start, __head_end, PTEFLAGS_READWRITE);
	map_kernel(kptes, __text_start, __text_end, PTEFLAGS_READEXEC);
	map_kernel(kptes, __rodata_start, __rodata_end, PTEFLAGS_READONLY);
	map_kernel(kptes, __data_start, __data_end, PTEFLAGS_READWRITE);
	map_kernel(kptes, __percpu_start, __percpu_end, PTEFLAGS_READWRITE);

	MMPTEP p1d = MmGetPteForAddress(0xffffffff80000000UL);
	MMPTEP p2d = MmGetPteForPtr(p1d);
	unsigned long addr = MiVirtToPhys((unsigned long) kptes);

	unsigned long num_to_clear = 512 - num_kernel_pgtables;
	for (; num_kernel_pgtables; num_kernel_pgtables--) {
		HalWritePTE(p2d, HalMakeTableKPTE(2, addr));
		p2d++;
		addr += PAGE_SIZE;
	}

	for (; num_to_clear; num_to_clear--) {
		HalClearPTE(p2d);
		p2d++;
	}

	map_start = 0;
	map_end = 0;
	for (int i = 0; i < memmap_len; i++) {
		multiboot_memmap_entry entry = memmap_entry(i);
		unsigned long start = entry.start;
		unsigned long end = start + entry.size;
		unsigned int type = entry.type;

		if (type != MB2_MEMMAP_USABLE && type != MB2_MEMMAP_ACPI_RECLAIM)
			continue;

		start = (unsigned long) MmGetPFNForPhys(start);
		end = (unsigned long) MmGetPFNForPhys(PGALIGN_UP(end));

		start = PGALIGN_DOWN(start);
		end = PGALIGN_UP(end);

		if (map_end == start) {
			map_end = end;
			continue;
		}

		if (map_start != map_end)
			map_pfn_range(map_start, map_end);

		map_start = start;
		map_end = end;
	}
	if (map_start != map_end)
		map_pfn_range(map_start, map_end);

	/*
	 * Zero the bottom-half top level PTEs, which correspond to user space.
	 */
	MMPTEP pgd = MmGetPteForAddressLevel(0, HalNumPageTableLevels());
	for (int i = 0; i < 256; i++) {
		HalClearPTE(pgd++);
	}
	/*
	 * Flush page translations globally by writing to CR4.
	 */
	write_cr4(__cr4_state ^ __CR4_PGE);
	write_cr4(__cr4_state);
}

static void debugcon_putstring(CONSOLE *console, const char *message)
{
	(void) console;
	io_outsb(0xe9, (const uint8_t *) message, strlen(message));
}

static CONSOLE debugcon = {
	.flags = CONSOLE_PANIC_CAPABLE,
	.putString = debugcon_putstring,
};

extern "C"
void HalStartKernel(void *multiboot_info, unsigned long kernel_load_offset,
		unsigned long percpu_offset)
{
	boot_params = (multiboot_params *) multiboot_info;
	load_offset = kernel_load_offset;
	Hal::percpu_offsets[0] = percpu_offset;

	cpufeature_init();
	if (CPUFeature(LA57))
		max_supported_ram = 1UL << 52;
	else
		max_supported_ram = 1UL << 46;

	if (x86_max_phys_addr < max_supported_ram)
		max_supported_ram = x86_max_phys_addr;

	if (io_inb(0xe9) == 0xe9)
		KeRegisterConsole(&debugcon);

	KiInitializeEarlySubsystems();
	init_idt();

	KePrintf("HalStartKernel: multiboot_info=%p, load_offset=0x%lx\n",
			multiboot_info, kernel_load_offset);

	KePrintf("CPU model: %s (%s)\n", cpu_model_string, cpu_brand_string);

	HalInitializeVirtualAddressSpace();

	/*
	 * Setup the self-mapping PTE in the highest page table.
	 *
	 * NB: we still have identity mappings for the lower 4 GiB of memory at
	 * this point, meaning that %cr3 == physical address == virtual address.
	 */
	MMPTEP ptep = (MMPTEP) read_cr3();
	MMPTE pte = { (unsigned long) ptep | __KERNEL_PAGE_TABLE };
	if (CPUFeature(NX))
		pte.value |= __PG_NX;
	HalWritePTE(ptep + MiSelfMappingPTEIndex, pte);

	init_memory();
	boot_params = (multiboot_params *) MiPhysToVirt((unsigned long) boot_params);
	mb2_memmap = (multiboot_memmap *) MiPhysToVirt((unsigned long) mb2_memmap);

	init_free_memory();

	HalInitializeVmapPageTables();

	KiStartKernel();

	KePanic("KiStartKernel returned");
}

