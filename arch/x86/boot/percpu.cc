/**
 * Initialization of per-CPU storage.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/irql.h>
#include <asm/pcpu_fixed.h>
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

PERCPU_CONSTRUCTOR(init_pcpu_fixed)
{
	x86_pcpu_fixed *p = percpu_ptr (__pcpu_fixed).on (cpu);
	p->pcpu_offset = (void *) pcpu_detail::offsets[cpu];
	p->cpu_id = cpu;
	p->irql_level[1] = __IRQL_NONE_PENDING;
	p->irql_level[2] = __IRQL_NONE_PENDING;
}
