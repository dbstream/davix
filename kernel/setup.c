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

	create_kernel_task("kernel task", mt_test, NULL);

	for(;;)
		relax();
}
