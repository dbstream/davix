/**
 * Kernel stack unwinding.
 * Copyright (C) 2025-present  dbstream
 *
 * Ok, unwinding is still TODO... for now, just demonstrate our ability to
 * read the embed data.
 */
#include <davix/printk.h>
#include <asm/sections.h>
#include <asm/unwind.h>

static inline unsigned long
readat (unsigned long offset)
{
	return *(unsigned long *) &__kernel_end[offset];
}

void
dump_backtrace (void)
{
	printk ("embed data: addr=0x%lx  data[0]=0x%lx  data[1]=0x%lx\n",
			(unsigned long) &__kernel_end[0], readat (0), readat (8));
}
