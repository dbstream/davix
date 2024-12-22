/**
 * Kernel main() function.
 * Copyright (C) 2024  dbstream
 */
#include <davix/context.h>
#include <davix/cpuset.h>
#include <davix/ktest.h>
#include <davix/main.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/smp.h>
#include <davix/task_api.h>
#include <davix/vmap.h>
#include <asm/irq.h>
#include <asm/page.h>
#include <asm/sections.h>

static const char kernel_version[] = KERNELVERSION;

/**
 * Start the init task.
 */
static void
start_init (void *arg)
{
	(void) arg;

	sched_begin_task ();
	printk (PR_INFO "start_init()\n");

	smp_boot_cpus ();

	run_ktests ();

	irq_disable ();
	get_current_task ()->state = TASK_ZOMBIE;
	schedule ();
}

/**
 * Kernel main function. Called in an architecture-specific way to startup
 * kernel subsystems and eventually transition into userspace.
 */
__INIT_TEXT
void
main (void)
{
	set_cpu_online (0);
	set_cpu_present (0);

	preempt_off ();
	printk (PR_NOTICE "Davix version %s\n", kernel_version);

	arch_init ();
	arch_init_pfn_entry ();
	smp_init ();

	vmap_init ();
	smpboot_init ();

	irq_enable ();

	tasks_init ();
	sched_init ();

	if (!create_kernel_task ("init", start_init, NULL))
		panic ("Couldn't create init task.");

	printk (PR_NOTICE "Reached end of main()\n");
	sched_idle ();
}
