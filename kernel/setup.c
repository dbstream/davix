/* SPDX-License-Identifier: MIT */
#include <davix/setup.h>
#include <davix/printk.h>

void start_kernel(const char *cmdline)
{
	info("Starting Davix kernel, version %s%s...\n",
		KERNELVERSION, CONFIG_EXTRAVERSION);

	arch_early_init();

	info("Done.\n");
	for(;;)
		relax();
}
