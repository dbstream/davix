/**
 * Initialization of per-CPU storage.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/pcpu_init.h>
#include <asm/percpu.h>

namespace pcpu_detail {

uintptr_t offsets[CONFIG_MAX_NR_CPUS];

}

extern "C" char __pcpu_constructors_start[];
extern "C" char __pcpu_constructors_end[];

void
call_pcpu_constructors_for (unsigned int cpu)
{
	void (*const *start) (unsigned int) = (void (*const *)(unsigned int)) __pcpu_constructors_start;
	void (*const *end) (unsigned int) = (void (*const *)(unsigned int)) __pcpu_constructors_end;
	asm volatile ("" : "+r" (start), "+r" (end) :: "memory");

	while (start != end) {
		void (*function) (unsigned int) = *start;
		function (cpu);
		start++;
	}
}
