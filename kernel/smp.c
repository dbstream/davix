/* SPDX-License-Identifier: MIT */
#include <davix/smp.h>
#include <davix/sched.h>
#include <davix/printk.h>

/*
 * arch_init_smp(): architecture-specific function that initializes the
 * 'cpu_slots' array and the 'num_cpu_slots' constant.
 */
void arch_init_smp(void);

void init_smp(void)
{
	arch_init_smp();
	for_each_logical_cpu(cpu) {
		if(!cpu->possible)
			return;
		if(cpu->online) {
			info("CPU#%u online\n", cpu->id);
		} else {
			info("CPU#%u %s\n", cpu->id,
				cpu->present ? "present" : "hot-pluggable");
		}
	}
}
