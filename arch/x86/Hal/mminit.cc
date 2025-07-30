// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/mminit.cc
 * Kernel memory initialization.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/fixed_mapping.h>
#include <Hal/page_tables.h>
#include <Ke/log.h>
#include <asm/clear_page.h>
#include <asm/invlpg.h>
#include <davix/export.h>
#include "internal.h"

/**
 * fixed_mapping_page: global array of page table entries for the fixed-mapping
 * space.
 */
extern "C" { MMPTE fixed_mapping_page alignas(0x1000) [512]; };

/**
 * MiUserAddressBegin: marks the first byte of the virtual address space that
 * can be allocated to userspace.
 */
unsigned long MiUserAddressBegin;
EXPORT_SYMBOL(MiUserAddressBegin)

/**
 * MiUserAddressEnd: marks the first byte after the end of the virtual address
 * space that can be allocated to userspace.
 */
unsigned long MiUserAddressEnd;
EXPORT_SYMBOL(MiUserAddressEnd)

/**
 * MiHHDMBase: offset from physical addresses to virtual mappings of those
 * physical addresses.
 */
unsigned long MiHHDMBase;
EXPORT_SYMBOL(MiHHDMBase)

/**
 * MiPFNBaseAddress: marks the base address of the MMPFN array.
 */
unsigned long MiPFNBaseAddress;

/**
 * MiVmapSpaceBegin: marks the first byte of vmap space.
 */
unsigned long MiVmapSpaceBegin;
EXPORT_SYMBOL(MiVmapSpaceBegin)

/**
 * MiVmapSpaceEnd: marks the first byte after the end of vmap space.
 */
unsigned long MiVmapSpaceEnd;
EXPORT_SYMBOL(MiVmapSpaceEnd)

/**
 * MiPTEBase: the start of the PTE space region.
 */
unsigned long MiPTEBaseAddress;
EXPORT_SYMBOL(MiPTEBaseAddress);

/**
 * MiPFNBase: the global MMPFN array pointer.
 */
MMPFN *MiPFNBase;
EXPORT_SYMBOL(MiPFNBase)

/**
 * MiPTEBase: the global MMPTE array pointer.
 */
_MMPTE_TAG *MiPTEBase;
EXPORT_SYMBOL(MiPTEBase);

/*
 * MiPTEBaseForLevelIndex: global MMPTE array bases for each page table level.
 *
 * Indices into this array are zero-indexed page table levels (offset by -1 to
 * the normal, one-indexed page table levels).
 *
 * (MiPTEBaseForLevelIndex[0] == MiPTEBase)
 */
_MMPTE_TAG *MiPTEBaseForLevelIndex[5];
EXPORT_SYMBOL(MiPTEBaseForLevelIndex);

/**
 * MiPTEAddressMask: the not-sign-extended part of a linear address.
 */
unsigned long MiPTEAddressMask;
EXPORT_SYMBOL(MiPTEAddressMask);

void HalInitializeVirtualAddressSpace(void)
{
	if (CPUFeature(LA57)) {
		MiUserAddressBegin		= 0x0000000000010000UL;
		MiUserAddressEnd		= 0x00ffffffffb00000UL;
		MiHHDMBase			= 0xff10000000000000UL;
		MiPFNBaseAddress		= 0xff90000000000000UL;
		MiVmapSpaceBegin		= 0xffa0000000000000UL;
		MiVmapSpaceEnd			= 0xffd0000000000000UL;
		MiPTEBaseAddress		= 0xffd0000000000000UL;
		MiPTEAddressMask		= 0x01ffffffffffffffUL;
	} else {
		MiUserAddressBegin		= 0x0000000000010000UL;
		MiUserAddressEnd		= 0x00007fffffb00000UL;
		MiHHDMBase			= 0xffff880000000000UL;
		MiPFNBaseAddress		= 0xffffc80000000000UL;
		MiVmapSpaceBegin		= 0xffffd00000000000UL;
		MiVmapSpaceEnd			= 0xffffe80000000000UL;
		MiPTEBaseAddress		= 0xffffe80000000000UL;
		MiPTEAddressMask		= 0x0000ffffffffffffUL;
	}

	MiPFNBase = (MMPFN *) MiPFNBaseAddress;
	MiPTEBase = (MMPTE *) MiPTEBaseAddress;

	MiPTEBaseForLevelIndex[0] = MiPTEBase;
	for (int i = 1; i < 5; i++) {
		/*
		 * NB: this loop deliberately assigns an extraneous value
		 * corresponding to the PML5 in the !LA57 case.  This doesn't
		 * matter as it is a bug to access MiPTEBaseForLevelIndex[4] in
		 * that case.
		 */
		MMPTEP ptep = MmGetPteForPtr(MiPTEBaseForLevelIndex[i - 1]);
		MiPTEBaseForLevelIndex[i] = ptep;
	}
}

void HalSetFixedMapping(int idx, unsigned long addr, PTEFLAGS flags)
{
	MMPTEP ptep = fixed_mapping_page + idx;
	MMPTE oldpte = HalReadPTE(ptep);
	MMPTE newpte = HalMakeKPTE(1, addr, flags);

	if (HalPTEUpdateNeedsFlush(oldpte, newpte)) {
		HalClearPTE(ptep);
		__invlpg(HalFixedMappingAddress(idx));
	}

	HalWritePTE(ptep, newpte);
}

void HalClearFixedMapping(int idx)
{
	MMPTEP ptep = fixed_mapping_page + idx;
	MMPTE oldpte = HalReadPTE(ptep);

	if (!HalPTEEmpty(oldpte)) {
		HalClearPTE(ptep);
		__invlpg(HalFixedMappingAddress(idx));
	}
}

static unsigned long (*page_alloc_function)(void) = nullptr;

void HalSetEarlyPageAllocationFunction(unsigned long (*function)(void))
{
	page_alloc_function = function;
}

static unsigned long alloc_early_pgtable(void)
{
	if (!page_alloc_function)
		KePanic("alloc_early_pgtable: page_alloc_function is NULL!");

	unsigned long phys = page_alloc_function();
	if (!phys)
		KePanic("alloc_early_pgtable: couldn't allocate memory!");

	/*
	 * NB: reuse the local APIC page for page table zeroing here.  This is
	 * fine, as this code runs really early and we haven't yet mapped the
	 * local APIC to anything.
	 */
	HalSetFixedMapping(HAL_FIXED_MAP_LOCAL_APIC, phys, PTEFLAGS_READWRITE);
	clear_page((void *) HalFixedMappingAddress(HAL_FIXED_MAP_LOCAL_APIC));
	return phys;
}

static MMPTEP get_pte_early(unsigned long addr, int level, int maxpgtlevel)
{
#if DEBUG_PAGETABLES
	BUG_ON(level < 1);
	BUG_ON(level > maxpgtlevel);
#endif

	for (int current = maxpgtlevel;; --current) {
		MMPTEP ptep = MmGetPteForAddressLevel(addr, current);
		if (current == level)
			return ptep;
		MMPTE pte = HalReadPTE(ptep);
		if (HalPTEEmpty(pte)) {
			pte = HalMakeTableKPTE(current, alloc_early_pgtable());
			HalWritePTE(ptep, pte);
		}
	}
}

void HalMapRangeHHDM(unsigned long addr, unsigned long end)
{
	int maxpgtlevel = HalNumPageTableLevels();
	int maxptelevel = HalMaxHugePTELevel();

	addr = PGALIGN_DOWN(addr);
	end = PGALIGN_UP(end);

	int level = 1;
	while (level < maxptelevel) {
		unsigned long pte_size = HalPTESize(level);
		unsigned long upper_size = HalPTESize(level + 1);

		unsigned long x = (addr + upper_size - 1) & ~(upper_size - 1);
		if (!x || end < x)
			break;

		while (addr < x) {
			unsigned long phy = MiVirtToPhys(addr);
			MMPTEP ptep = get_pte_early(addr, level, maxpgtlevel);
			MMPTE pte = HalMakeKPTE(level, phy, PTEFLAGS_READWRITE);

			HalWritePTE(ptep, pte);
			addr += pte_size;
		}

		level++;
	}

	for (; level > 0; --level) {
		unsigned long pte_size = HalPTESize(level);

		unsigned long x = end & ~(pte_size - 1);

		while (addr < x) {
			unsigned long phy = MiVirtToPhys(addr);
			MMPTEP ptep = get_pte_early(addr, level, maxpgtlevel);
			MMPTE pte = HalMakeKPTE(level, phy, PTEFLAGS_READWRITE);

			HalWritePTE(ptep, pte);
			addr += pte_size;
		}
	}
}

