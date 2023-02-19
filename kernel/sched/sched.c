/* SPDX-License-Identifier: MIT */
#include <davix/sched.h>
#include <davix/kmalloc.h>
#include <davix/panic.h>

struct task *idle_task cpulocal;

void setup_sched_on(struct logical_cpu *cpu)
{
	struct task *idle = kmalloc(sizeof(struct task));
	if(!idle)
		panic("setup_sched_on(CPU#%u): couldn't allocate memory for idle task.",
			cpu->id);

	idle->flags = TASK_IDLE;
	arch_init_idle_task(cpu, idle);
	rdwr_cpulocal(idle_task) = idle;
}
