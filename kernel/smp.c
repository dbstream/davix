/**
 * SMP support
 * Copyright (C) 2024  dbstream
 */
#include <davix/context.h>
#include <davix/cpuset.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/smp.h>
#include <davix/stdbool.h>
#include <davix/workqueue.h>
#include <asm/irq.h>
#include <asm/sections.h>
#include <asm/smpboot.h>

unsigned int nr_cpus = 1;

cpuset_t cpus_online;
cpuset_t cpus_present;

static bool disable_smpboot = false;

__INIT_TEXT
void
smp_init (void)
{
	printk (PR_INFO "CPUs: %u\n", nr_cpus);
}

__INIT_TEXT
void
smpboot_init (void)
{
	if (nr_cpus == 1)
		return;

	errno_t e = arch_smpboot_init ();
	if (e != ESUCCESS) {
		printk (PR_ERR "smp: arch_smpboot_init failed with error %d\n", e);
		printk (PR_WARN "smp: will not attempt SMPBOOT\n");
		disable_smpboot = true;
	}
}

__INIT_TEXT
void
smp_boot_cpus (void)
{
	if (disable_smpboot)
		return;

	for_each_present_cpu (cpu) {
		if (cpu_online (cpu))
			continue;

		errno_t e = arch_smp_boot_cpu (cpu);
		if (e != ESUCCESS)
			printk (PR_ERR "smp: smp_boot_cpu(%u) failed with error %d\n", cpu, e);
	}
}

void
smp_start_additional_cpu (void)
{
	preempt_off ();
	irq_enable ();

	sched_init_this_cpu ();
	workqueue_init_this_cpu ();

	sched_idle ();
}
