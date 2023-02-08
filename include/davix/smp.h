/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_SMP_H
#define __DAVIX_SMP_H

#include <davix/types.h>
#include <asm/smp.h>

struct logical_cpu {
	unsigned long cpulocal_offset;
	unsigned id;
	bool online;
	bool possible;
	bool present;
};

/*
 * An array of logical CPUs, allocated by init_smp().
 */
extern struct logical_cpu *cpu_slots;

/*
 * This is the maximum number of logical CPUs, and the length of the above
 * array, determined at init_smp()-time.
 */
extern unsigned num_cpu_slots;

/*
 * Helpful macro.
 */
#define for_each_logical_cpu(cpu) \
	for(struct logical_cpu *cpu = &cpu_slots[0]; \
		cpu < &cpu_slots[num_cpu_slots]; cpu++)

void init_smp(void);

#define cpulocal_address(cpu, var) \
	((force typeof(var))((force unsigned long) (var) + (cpu)->cpulocal_offset))

#define rdwr_cpulocal_on(cpu, var) (*(cpulocal_address((cpu), &(var))))

#define rdwr_cpulocal(var) rdwr_cpulocal_on(smp_self(), (var))

#endif /* __DAVIX_SMP_H */
