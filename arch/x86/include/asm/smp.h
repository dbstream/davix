/**
 * SMP
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

static inline int
this_cpu_id (void)
{
	int cpu_id;
	asm volatile ("movl %%gs:8, %0" : "=r" (cpu_id) :: "memory");
	return cpu_id;
}
