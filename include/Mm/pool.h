// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Mm/pool.h
 * Interface to the kernel object allocator.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <new>

void MmInitializeObjectAllocator(void);

void *MmAllocateObject(unsigned long size, unsigned long align = 0);

void MmFreeObject(void *mem);

template<class T>
static inline T *MmNew(void)
{
	T *object = (T *) MmAllocateObject(sizeof(T), alignof(T));
	if (object)
		return new (object) T;
	else
		return nullptr;
}

template<class T>
static inline void MmDelete(T *object)
{
	if (object) {
		object->~T();
		MmFreeObject((void *) object);
	}
}

