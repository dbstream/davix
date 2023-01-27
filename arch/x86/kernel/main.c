/* SPDX-License-Identifier: MIT */
#include <davix/acpi.h>
#include <davix/printk.h>
#include <davix/list.h>
#include <davix/setup.h>
#include <asm/boot.h>
#include <asm/entry.h>
#include "uart.h"

struct boot_struct x86_boot_struct section(".bootstruct") = {
	.bootstruct_magic = BOOTSTRUCT_MAGIC,
	.protocol_version = BOOTPROTOCOL_VERSION_1_0_0,
	.kernel = "Davix kernel (early development version)",
	.memmap_entries = LIST_INIT(x86_boot_struct.memmap_entries),
	.l5_paging_enable = 0,
	.com1_initialized = 0,
	.com2_initialized = 0,
	.initrd_start = 0,
	.initrd_size = 0,
	.acpi_rsdp = 0,
	.cmdline = NULL
};

void x86_start_kernel(void);
void x86_start_kernel(void)
{
	x86_uart_init();
	x86_setup_early_idt();
	start_kernel();
}

static void init_early_acpi(void)
{
	acpi_status err = acpi_initialize_subsystem();
	if(err != AE_OK) {
		error("acpi_initialize_subsystem(): error %u\n", err);
		return;
	}

	err = acpi_initialize_tables(NULL, 0, 1);
	if(err != AE_OK) {
		error("acpi_initialize_tables(): error %u\n", err);
		return;
	}
}

void x86_setup_memory(void);	/* in arch/x86/mm/setup_memory.c */

void arch_early_init(void)
{
	info("x86/kernel: Booted with protocol version %u.%u.%u by \"%s\". LA57: %s\n",
		BOOTPROTOCOL_VERSION_MAJOR(x86_boot_struct.protocol_version),
		BOOTPROTOCOL_VERSION_MINOR(x86_boot_struct.protocol_version),
		BOOTPROTOCOL_VERSION_PATCH(x86_boot_struct.protocol_version),
		x86_boot_struct.bootloader_name,
		x86_boot_struct.l5_paging_enable ? "on" : "off");

	x86_setup_memory();

	if(x86_boot_struct.acpi_rsdp) {
		info("ACPI RSDP: %p\n", x86_boot_struct.acpi_rsdp);
	} else {
		info("No ACPI RSDP provided by bootloader.\n");
	}

	if(x86_boot_struct.initrd_size == 0) {
		info("No initial ramdisk provided.\n");
	} else {
		info("Initial ramdisk: [mem %p - %p]\n",
			x86_boot_struct.initrd_start,
			x86_boot_struct.initrd_start
				+ x86_boot_struct.initrd_size - 1);
	}

	init_early_acpi();
}
