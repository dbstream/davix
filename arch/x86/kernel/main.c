/* SPDX-License-Identifier: MIT */
#include <davix/printk.h>
#include <davix/list.h>
#include <asm/boot.h>
#include "uart.h"

struct boot_struct x86_boot_struct section(".bootstruct") = {
	.bootstruct_magic = BOOTSTRUCT_MAGIC,
	.protocol_version = BOOTPROTOCOL_VERSION_1_0_0,
	.kernel = "Davix kernel (early development version)",
	.memmap_entries = LIST_INIT(x86_boot_struct.memmap_entries),
	.l5_paging_enable = 0,
	.com1_initialized = 0,
	.com2_initialized = 0
};

void x86_setup_memory(void);	/* in arch/x86/mm/setup_memory.c */

void x86_start_kernel(void);
void x86_start_kernel(void)
{
	x86_uart_init();

	info("x86/kernel: Booted with protocol version %u.%u.%u by \"%s\". LA57: %s\n",
		BOOTPROTOCOL_VERSION_MAJOR(x86_boot_struct.protocol_version),
		BOOTPROTOCOL_VERSION_MINOR(x86_boot_struct.protocol_version),
		BOOTPROTOCOL_VERSION_PATCH(x86_boot_struct.protocol_version),
		x86_boot_struct.bootloader_name,
		x86_boot_struct.l5_paging_enable ? "on" : "off");

	x86_setup_memory();

	info("x86/kernel: Done.\n");
	for(;;)
		__builtin_ia32_pause();
}
