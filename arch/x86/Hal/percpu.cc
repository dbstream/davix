// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/percpu.cc
 * HAL support routines for percpu variables.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/percpu.h>
#include <Ke/log.h>
#include <Mm/vmap.h>
#include <string.h>

struct KTHREAD;

namespace Hal {
	unsigned long percpu_offsets[CONFIG_MAX_NR_CPUS];
}

extern "C" char __percpu_callbacks_start[];
extern "C" char __percpu_callbacks_end[];

extern "C" char __percpu_start[];
extern "C" char __percpu_end[];

/**
 * HalInitializePerCPUVariables - call percpu variable initialization routines.
 * @cpu: CPU for which to call percpu variable initialization routines.
 */
void HalInitializePerCPUVariables(unsigned int cpu)
{
	auto start = (void (*const *)(unsigned int)) __percpu_callbacks_start;
	auto end = (void (*const *)(unsigned int)) __percpu_callbacks_end;
	asm("" : "+r"(start), "+r"(end));

	for (; start != end; start++)
		(*start)(cpu);
}

/*
 * NOTE: KEEP IN SYNC WITH VALUES IN startup.S!!!
 */
struct percpu_fixed {
	unsigned long self_ptr;
	unsigned int cpu;
	char pad1[4];
	KTHREAD *thread;
	char pad2[16];
	unsigned long stack_guard_val;
	char pad3[16];
};

/**
 * HalAllocateAndInitializePerCPUVariables - allocate and initialize the percpu
 * storage region for a CPU.
 * @cpu: CPU for which to initialize per-CPU variables storage.
 */
void HalAllocateAndInitializePerCPUVariables(unsigned int cpu)
{
	unsigned long start = (unsigned long) __percpu_start;
	unsigned long end = (unsigned long) __percpu_end;
	asm("" : "+r"(start), "+r"(end));

	unsigned long size = end - start;
	void *mem = MmAllocateVirtual(size, PTEFLAGS_READWRITE);

	if (!mem)
		KePanic("HalAllocateAndInitializePerCPUVariables: out of memory");
	memset(mem, 0, size);
	Hal::percpu_offsets[cpu] = (unsigned long) mem;

	percpu_fixed *pcpu_fixed = (percpu_fixed *) mem;
	pcpu_fixed->self_ptr = (unsigned long) mem;
	pcpu_fixed->cpu = cpu;
	pcpu_fixed->stack_guard_val = 0;

	HalInitializePerCPUVariables(cpu);
}

extern "C" char __percpu_GDT[];

HAL_PERCPU_CALLBACK(cpu)
{
	if (cpu == 0)
		return;

	char *target = HalPtrPerCPU(__percpu_GDT[0], cpu);
	char *source = HalPtrThisCpu(__percpu_GDT[0]);

	memcpy(target, source, 0x40);
}

