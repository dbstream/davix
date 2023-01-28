/* SPDX-License-Identifier: MIT */
#include <davix/setup.h>
#include <davix/smp.h>
#include <davix/panic.h>
#include <davix/printk.h>

const char *kernel_cmdline;

void start_kernel(void)
{
	info("Starting Davix kernel, version %s%s...\n",
		KERNELVERSION, CONFIG_EXTRAVERSION);

	arch_early_init();

	/* By now, ``kernel_cmdline`` should be set. */
	info("Kernel command line: \"%s\"\n", kernel_cmdline);

	init_smp();

	panic("TODO: Implement the rest of the kernel.");
}
