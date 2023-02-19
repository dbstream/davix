/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_SCHED_H
#define __DAVIX_SCHED_H

#include <davix/smp.h>
#include <asm/task.h>

typedef bitwise int task_flags_t;
#define __TF(x) ((force task_flags_t) (1 << x))

#define TASK_IDLE __TF(0) /* The task is an idle task. */

struct task {
	struct arch_task_info arch_task_info;

	/*
	 * Task flags.
	 */
	task_flags_t flags;
};

extern struct task *idle_task cpulocal;

/*
 * Perform architecture-specific initialization of an idle task.
 */
void arch_init_idle_task(struct logical_cpu *cpu, struct task *task);

/*
 * Initialize basic scheduler data structures on a particular CPU.
 */
void setup_sched_on(struct logical_cpu *cpu);

#endif /* __DAVIX_SCHED_H */
