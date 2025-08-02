// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Mm/pool.h
 * Interface to the kernel object allocator.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

void MmInitializeObjectAllocator(void);

void *MmAllocateObject(unsigned long size, unsigned long align = 0);

void MmFreeObject(void *mem);

template<class T>
static inline T *MmNew(void)
{
	return (T *) MmAllocateObject(sizeof(T), alignof(T));
}

template<class T>
static inline void MmDelete(T *object)
{
	MmFreeObject((void *) object);
}

