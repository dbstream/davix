/* SPDX-License-Identifier: MIT */
#ifndef __MEMALLOC_H
#define __MEMALLOC_H

#include "memmap.h"

extern unsigned long highest_mapped_address;

void *__memalloc(unsigned long size, unsigned long align,
	unsigned long start, unsigned long end, enum memmap_type type);

static inline void *_memalloc(unsigned long size, unsigned long align,
	unsigned long start, unsigned long end, enum memmap_type type)
{
	end = min(end, highest_mapped_address);
	if(end <= start)
		return NULL;
	return __memalloc(size, align, start, end, type);
}

static inline void *memalloc(unsigned long size, unsigned long align,
	enum memmap_type type)
{
	void *ret = _memalloc(size, align,
		0x100000000UL, 0xfffffffffffff000UL, type);
	if(!ret)
		ret = _memalloc(size, align,
			0x1000000, 0xfffffffffffff000UL, type);
	if(!ret)
		ret = _memalloc(size, align,
			0, 0xfffffffffffff000UL, type);
	return ret;
}

static inline void *memzalloc(unsigned long size, unsigned long align,
	enum memmap_type type)
{
	void *ret = memalloc(size, align, type);
	if(ret)
		memset(ret, 0, size);
	return ret;
}

#endif /* __MEMALLOC_H */
