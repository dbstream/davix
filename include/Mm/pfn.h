// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Mm/pfn.h
 * MMPFN structure.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Hal/mm.h>
#include <dsl/list.h>

typedef struct _MMPFN_TAG {
	/*
	 * pg_list: MMPFN_List linkage.
	 */
	dsl::ListHead pg_list;
	/*
	 * pg_ptr: depends on page usage.
	 *
	 * MIOBJECTPOOL page: pointer to MIOBJECTPOOL.
	 */
	void *pg_ptr;
	/*
	 * u: MMPFN union (two words).
	 */
	union {
		struct {
			/*
			 * u.pool.count: free object count
			 */
			unsigned int count;
			/*
			 * u.pool.obj_head: pointer to first free object
			 */
			void *obj_head;
		} pool;
	} u;
	long pad[3];
} MMPFN;

#ifdef __x86_64__
static_assert(sizeof(MMPFN) == 64, "MMPFN has the wrong size!");
#endif

/**
 * MmGetPFNForPhys - get the MMPFN that corresponds to a physical address.
 * @phys: physical address
 */
static inline MMPFN *MmGetPFNForPhys(unsigned long phys)
{
	return &MiPFNBase[phys / PAGE_SIZE];
}

/**
 * MmGetPhysForPFN - get the physical address of a MMPFN-managed page.
 * @pfn: pointer to MMPFN structure
 */
static inline unsigned long MmGetPhysForPFN(MMPFN *pfn)
{
	return PAGE_SIZE * (pfn - MiPFNBase);
}

/**
 * MiGetPFNForVirt - get the MMPFN that corresponds to a direct-mapped address.
 * @virt: virtual address
 */
static inline MMPFN *MiGetPFNForVirt(unsigned long virt)
{
	return MmGetPFNForPhys(MiVirtToPhys(virt));
}

/**
 * MiGetVirtForPFN - get the direct-mapped address of a MMPFN-managed page.
 */
static inline unsigned long MiGetVirtForPFN(MMPFN *pfn)
{
	return MiPhysToVirt(MmGetPhysForPFN(pfn));
}

typedef dsl::TypedList<_MMPFN_TAG, &_MMPFN_TAG::pg_list> MMPFN_List;

