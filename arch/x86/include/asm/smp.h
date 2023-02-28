/* SPDX-License-Identifier: MIT */
#ifndef __ASM_SMP_H
#define __ASM_SMP_H

#include <davix/types.h>

#define cpulocal section(".cpulocal")

struct logical_cpu;

extern struct logical_cpu *__smp_self cpulocal;

static inline struct logical_cpu *smp_self(void)
{
	struct logical_cpu *ret;
	asm volatile("movq %%gs:0, %0" : "=r"(ret) : : "memory");
	return ret;
}

extern unsigned *acpi_cpu_to_lapic_id;

#endif /* __ASM_SMP_H */
