/**
 * Runtime allocation of CPU-local storage.
 * Copyright (C) 2025-present  dbstream
 *
 * This file implements dynamic allocation of CPU-local storage at runtime.  It
 * can be used by e.g. slab to implement fast CPU-local object bins.
 */
#include <davix/atomic.h>
#include <davix/align.h>
#include <davix/cpulocal.h>
#include <davix/spinlock.h>
#include <davix/printk.h>

/**
 * Currently this is implemented as a bump allocator.  This can be improved
 * later.
 */

static unsigned long cpulocal_free_start, cpulocal_free_end;
static spinlock_t cpulocal_rt_lock;

void
cpulocal_rt_init (unsigned long start, unsigned long end)
{
	printk (PR_INFO "cpulocal_rt:  start=0x%lx  end=0x%lx  size=%lu bytes\n",
			start, end, end - start);

	spin_lock (&cpulocal_rt_lock);
	cpulocal_free_start = start;
	cpulocal_free_end = end;
	spin_unlock (&cpulocal_rt_lock);
}

void *
cpulocal_rt_alloc (unsigned long size, unsigned long align)
{
	if (align < 8)
		align = 8;

	void *ret = 0;
	spin_lock (&cpulocal_rt_lock);

	unsigned long x = ALIGN_UP (cpulocal_free_start, align);
	if (x && x + size < x)
		x = 0;
	else if (x)
		x += size;

	if (x <= cpulocal_free_end) {
		ret = (void *) x - size;
		cpulocal_free_start = x;
	}

	spin_unlock (&cpulocal_rt_lock);
	return ret;
}
