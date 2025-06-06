/**
 * Simultaneous Multiprocessing (SMP) support.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/cpuset.h>
#include <davix/printk.h>
#include <davix/smp.h>

void
smp_boot_all_cpus (void)
{
	if (nr_cpus == 1)
		return;

	printk (PR_NOTICE "SMP: Bringing %u additional CPU(s) online...\n", nr_cpus - 1);

	unsigned int nr_onlined = 0;
	for (unsigned int cpu : cpu_present) {
		if (cpu_online (cpu))
			continue;

		if (!arch_smp_boot_cpu (cpu)) {
			printk (PR_ERROR "SMP: Failed to bring CPU%u online; cancelling SMPBOOT\n", cpu);
			break;
		}

		nr_onlined++;
	}

	printk (PR_NOTICE "SMP: Brought %u additional CPU(s) online\n", nr_onlined);
}
