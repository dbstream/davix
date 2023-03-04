/* SPDX-License-Identifier: MIT */
#include <davix/setup.h>
#include <davix/smp.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/time.h>

const char *kernel_cmdline;

void mt_test(void *arg);
void mt_test(void *arg)
{
	(void) arg;
	info("\"Hello, World!\" from a kernel task!\n");
}

void start_kernel(void)
{
	info("Starting Davix kernel, version %s%s...\n",
		KERNELVERSION, CONFIG_EXTRAVERSION);

	arch_early_init();

	/* By now, ``kernel_cmdline`` should be set. */
	info("Kernel command line: \"%s\"\n", kernel_cmdline);

	time_init();
	init_smp();
	sched_init();

	arch_setup_interrupts();
	enable_interrupts();

	/*
	 * start_kernel() is entered with a preempt_disabled count of 1. Now that
	 * interrupt handling is setup and the scheduler is ready, we can enable
	 * kernel preemption.
	 */
	preempt_enable();

	create_kernel_task("kernel task", mt_test, NULL);

	struct sched_timer timer;
	create_sched_timer(&timer, ns_since_boot() + 1000000000UL);
	preempt_disable();
	info("Waiting for a sched timer...\n");
	set_task_flag(current_task(), TASK_GOING_TO_SLEEP);
	set_task_state(current_task(), TASK_UNINTERRUPTIBLE);
	sched_timer_wait(&timer);
	info("Waited for a sched timer.\n");
	preempt_enable();
	destroy_sched_timer(&timer);

	set_task_state(current_task(), TASK_UNINTERRUPTIBLE);
	schedule();
	panic("schedule() returned to an uninterruptible task!");
}
