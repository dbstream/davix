// SPDX-License-Identifier: GPL-3.0
/*
 * File: Mm/pool.cc
 * Kernel object allocator.
 *
 * Copyright (C) 2025  dbstream
 *
 * This is essentially the kernel's malloc.
 */
#include <Ke/log.h>
#include <Ke/spinlock.h>
#include <Mm/page_alloc.h>
#include <Mm/pfn.h>
#include <Mm/pool.h>
#include <davix/bug.h>
#include <lock_guard.h>

struct MIOBJECTPOOL {
	/*
	 * MIOBJECTPOOL spinlock. Held during MIOBJECTPOOL operations.
	 */
	KeSpinlock lock;
	/*
	 * List of pages which are not starved of free objects.
	 */
	MMPFN_List page_list;
	/*
	 * Object size in bytes.  Must be a power-of-two and bigger than a long.
	 */
	unsigned long obj_size;
	/*
	 * Object address mask.  == ~(object_size - 1)
	 *
	 * This is used during MmFreeObject to mask the passed pointer so it can
	 * point to anywhere within the object memory.
	 */
	unsigned long obj_addr_mask;
	/*
	 * The number of objects per page.
	 */
	unsigned int objs_per_page;
};

struct MIFREEOBJECT {
	MIFREEOBJECT *next;
};

/**
 * MiInitializePool - initialize an object pool.
 * @pool: pool to initialize
 * @size: object size in bytes
 */
static void MiInitializePool(MIOBJECTPOOL *pool, unsigned long size)
{
	BUG_ON(size < sizeof(long));		// size must be at least a long
	BUG_ON(size > 256 * sizeof(long));	// size must be below this limit
	BUG_ON(size & (size - 1));		// size must be a power-of-two

	pool->lock.init();
	pool->page_list.init();
	pool->obj_size = size;
	pool->obj_addr_mask = ~(size - 1);
	pool->objs_per_page = PAGE_SIZE / size;
}

/**
 * MiPoolAllocate - allocate an object from an object pool.
 * @pool: pointer to MIOBJECTPOOL
 */
static inline void *MiPoolAllocate(MIOBJECTPOOL *pool)
{
	scoped_lock_guard guard(pool->lock);
	if (pool->page_list.empty()) {
		[[unlikely]];
		MMPFN *pfn = MmAllocatePage();
		if (!pfn) {
			[[unlikely]];
			return nullptr;
		}

		unsigned long virt = MiGetVirtForPFN(pfn);
		unsigned long offset = 0;
		unsigned long obj_size = pool->obj_size;
		MIFREEOBJECT *prev = nullptr;
		do {
			MIFREEOBJECT *obj = (MIFREEOBJECT *) (virt + offset);
			obj->next = prev;
			prev = obj;
			offset += obj_size;
		} while (offset < PAGE_SIZE);
		pfn->pg_ptr = pool;
		pfn->u.pool.count = pool->objs_per_page - 1;
		pfn->u.pool.obj_head = prev->next;
		pool->page_list.push_front(pfn);
		return prev;
	}

	MMPFN *pfn = pool->page_list.first();
	MIFREEOBJECT *obj = (MIFREEOBJECT *) pfn->u.pool.obj_head;
	pfn->u.pool.count--;
	pfn->u.pool.obj_head = obj->next;

	if (pfn->u.pool.count == 0)
		pfn->pg_list.remove();

	return obj;
}

/**
 * MiPoolFree - free an object to an object pool.
 * @mem: pointer to object memory
 * @pool: pointer to MIOBJECTPOOL
 * @pfn: pointer to MMPFN
 */
static inline void MiPoolFree(void *mem, MIOBJECTPOOL *pool, MMPFN *pfn)
{
	unsigned long obj_addr = (unsigned long) mem;
	obj_addr &= pool->obj_addr_mask;

	MIFREEOBJECT *obj = (MIFREEOBJECT *) obj_addr;

	scoped_lock_guard guard(pool->lock);
	obj->next = (MIFREEOBJECT *) pfn->u.pool.obj_head;
	pfn->u.pool.count++;
	pfn->u.pool.obj_head = obj;

	if (pfn->u.pool.count == 1) {
		pool->page_list.push_front(pfn);
	} else if (pfn->u.pool.count == pool->objs_per_page) {
		pfn->pg_list.remove();
		guard.drop();
		MmFreePage(pfn);
	}
}

static MIOBJECTPOOL p2size_pools[9];

/*
 * MmInitializeObjectAllocator - initialize the object allocator.
 */
void MmInitializeObjectAllocator(void)
{
	for (int i = 0; i < 9; i++) {
		MiInitializePool(&p2size_pools[i], sizeof(long) << i);
	}
}

/**
 * MiGetPoolForAllocationSize - get the MIOBJECTPOOL to use for an object type.
 * @size: object size in bytes
 * @align: object alignment in bytes
 */
static inline MIOBJECTPOOL *MiGetPoolForAllocationSize(
		unsigned long size,
		unsigned long align
)
{
	/*
	 * MIOBJECTPOOLs allocate in power-of-two sizes, so this is enough to
	 * provide object alignment.
	 */
	if (align > size)
		size = align;
	/*
	 * This is the maximum allowed allocation size for MIOBJECTPOOL memory.
	 */
	if (size > 256 * sizeof(long))
		return nullptr;
	/*
	 * Find the MIOBJECTPOOL to use for this allocation.
	 */
	int i = 0;
	unsigned long x = sizeof(long);
	while (size > x) {
		i++;
		x += x;
	}

	return &p2size_pools[i];
}

/**
 * MmAllocateObject - allocate an object from the global MIOBJECTPOOLs.
 * @size: object size in bytes
 * @align: object alignment in bytes
 */
void *MmAllocateObject(unsigned long size, unsigned long align)
{
	MIOBJECTPOOL *pool = MiGetPoolForAllocationSize(size, align);
	if (!pool) {
		[[unlikely]];
		KePrintf("MmAllocateObject: allocation size %lu bytes alignment %lu bytes is invalid\n",
				size, align);
		return nullptr;
	}

	return MiPoolAllocate(pool);
}

/**
 * MmFreeObject - free an object to its MIOBJECTPOOL.
 * @mem: object pointer
 */
void MmFreeObject(void *mem)
{
	if (!mem) {
		[[unlikely]];
		return;
	}

	MMPFN *pfn = MiGetPFNForVirt((unsigned long) mem);
	MIOBJECTPOOL *pool = (MIOBJECTPOOL *) pfn->pg_ptr;

	MiPoolFree(mem, pool, pfn);
}

