/**
 * arch_init
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/page_defs.h>
#include <asm/time.h>
#include <davix/printk.h>
#include <davix/start_kernel.h>
#include <uacpi/uacpi.h>

void
arch_init (void)
{
	/**
	 * Setup early table access.
	 * NB: this uses the boot_pagetables memory as the temporary buffer.
	 */
	uacpi_setup_early_table_access ((void *) (KERNEL_START + 0x4000), 0x2000UL);

	x86_init_time ();
}
