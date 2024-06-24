/**
 * Kernel main() function.
 * Copyright (C) 2024  dbstream
 */
#include <davix/cpuset.h>
#include <davix/initmem.h>
#include <davix/main.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/slab.h>
#include <davix/smp.h>
#include <davix/time.h>
#include <asm/irq.h>
#include <asm/page.h>
#include <asm/sections.h>

static const char kernel_version[] = KERNELVERSION;

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

	printk (PR_NOTICE "Davix version %s\n", kernel_version);

	arch_init ();
	arch_init_pfn_entry ();
	smp_init ();

	irq_enable ();

	printk (PR_NOTICE "Reached end of main()\n");
	for (;;)
		arch_wfi ();
}
