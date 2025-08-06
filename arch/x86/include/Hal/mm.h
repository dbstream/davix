// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/mm.h
 * HAL memory model.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

/**
 * DOC: memory layout on x86_64
 *
 * Memory layout with 4-level page tables (LA48):
 *
 *   0x0000000000000000 - 0x0000000000010000    64 KiB  Guard hole.
 *   0x0000000000000000 - 0x00007fffffb00000  ~128 TiB  Available to userspace.
 *   0x00007fffffb00000 - 0x0000800000000000     4 MiB  Guard hole.
 *   0x0000800000000000 - 0xffff800000000000   ~16 EiB  Noncanonical hole.
 *   0xffff800000000000 - 0xffff880000000000     8 TiB  Reserved (for Xen dom0 kernels?).
 *   0xffff880000000000 - 0xffffc80000000000    64 TiB  Higher-Half Direct Map.
 *   0xffffc80000000000 - 0xffffc90000000000     1 TiB  MMPFN array.
 *   0xffffc90000000000 - 0xffffd00000000000     7 TiB  Reserved.
 *   0xffffd00000000000 - 0xffffe80000000000    24 TiB  vmap space.
 *   0xffffe80000000000 - 0xffffe88000000000   0.5 TiB  PTE space.
 *   0xffffe88000000000 - 0xffffffff80000000   ~24 TiB  Reserved.
 *   0xffffffff80000000 - 0xffffffffffffffff     2 GiB  Shared with LA57 layout.
 *
 * Memory layout with 5-level page tables (LA57):
 *
 *   0x0000000000000000 - 0x0000000000010000    64 KiB  Guard hole.
 *   0x0000000000000000 - 0x00ffffffffb00000   ~64 PiB  Available to userspace.
 *   0x00ffffffffb00000 - 0x0100000000000000     4 MiB  Guard hole.
 *   0x0100000000000000 - 0xff00000000000000   ~16 EiB  Noncanonical hole.
 *   0xff00000000000000 - 0xff10000000000000     4 PiB  Reserved (for Xen dom0 kernels?).
 *   0xff10000000000000 - 0xff90000000000000    32 PiB  Higher-Half Direct Map.
 *   0xff90000000000000 - 0xff92000000000000   512 TiB  MMPFN array.
 *   0xff92000000000000 - 0xffa0000000000000   3.5 PiB  Reserved.
 *   0xffa0000000000000 - 0xffd0000000000000    12 PiB  vmap space.
 *   0xffd0000000000000 - 0xffd1000000000000   256 TiB  PTE space.
 *   0xffd1000000000000 - 0xffffffff80000000   ~12 PiB  Reserved.
 *   0xffffffff80000000 - 0xffffffffffffffff     2 GiB  Shared with LA48 layout.
 *
 * High address ranges, used by both 4-level and 5-level page table memory maps:
 *
 *   0xffffffff80000000 - 0xffffffffc0000000     1 GiB  Kernel and modules space.
 *   0xffffffffc0000000 - 0xffffffffff000000  1008 MiB  Reserved.
 *   0xffffffffff000000 - 0xffffffffff200000     2 MiB  Fixed mapping space.
 *   0xffffffffff200000 - 0xffffffffffffffff    14 MiB  Guard hole.
 */

struct _MMPFN_TAG;
struct _MMPTE_TAG;

static constexpr unsigned long PAGE_SIZE = 4096UL;
static constexpr int PAGE_SHIFT = 12;

static constexpr unsigned long PGALIGN_DOWN(unsigned long address)
{
	return address & ~(PAGE_SIZE - 1);
}

static constexpr unsigned long PGALIGN_UP(unsigned long address)
{
	return (address + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

extern unsigned long MiUserAddressBegin;

extern unsigned long MiUserAddressEnd;

extern unsigned long MiHHDMBase;

extern unsigned long MiPFNBaseAddress;

extern unsigned long MiVmapSpaceBegin;

extern unsigned long MiVmapSpaceEnd;

extern unsigned long MiPTEBaseAddress;

static constexpr int MiSelfMappingPTEIndex = 464;

extern _MMPFN_TAG *MiPFNBase;
extern _MMPTE_TAG *MiPTEBase;

extern _MMPTE_TAG *MiPTEBaseForLevelIndex[];

/**
 * MiPhysToVirt - get the direct-map virtual address of a physical address.
 * @phys_addr: physical address
 */
static inline unsigned long MiPhysToVirt(unsigned long phys_addr)
{
	return phys_addr + MiHHDMBase;
}

/**
 * MiVirtToPhys - get the physical address of a direct-map virtual address.
 * @virt_addr: a direct-mapped virtual address
 */
static inline unsigned long MiVirtToPhys(unsigned long virt_addr)
{
	return virt_addr - MiHHDMBase;
}

static inline bool MmIsVmapPointer(void *ptr)
{
	unsigned long addr = (unsigned long) ptr;
	return addr >= MiVmapSpaceBegin && addr < MiVmapSpaceEnd;
}

