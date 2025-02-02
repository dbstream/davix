/**
 * Runtime allocation of CPU-local storage.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_CPULOCAL_H
#define _DAVIX_CPULOCAL_H 1

/**
 * Initialize the runtime CPU-local memory manager.
 *
 * @start	start of the region that can be allocated from
 * @end		end of the region that can be allocated from
 */
extern void
cpulocal_rt_init (unsigned long start, unsigned long end);

/**
 * Allocate CPU-local memory at runtime.
 *
 * @size	number of bytes to allocate
 * @align	minimum alignment of the start of the region to allocate
 */
extern void *
cpulocal_rt_alloc (unsigned long size, unsigned long align);

#endif /* _DAVIX_CPULOCAL_H  */
