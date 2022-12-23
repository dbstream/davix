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

void x86_start_kernel(void);
void x86_start_kernel(void)
{
	x86_uart_init();

	info("x86/kernel: Booted with protocol version %u.%u.%u by \"%s\".\n",
		BOOTPROTOCOL_VERSION_MAJOR(x86_boot_struct.protocol_version),
		BOOTPROTOCOL_VERSION_MINOR(x86_boot_struct.protocol_version),
		BOOTPROTOCOL_VERSION_PATCH(x86_boot_struct.protocol_version),
		x86_boot_struct.bootloader_name);

	struct memmap_entry *entry;

	info("x86/kernel: Memory map:\n");
	list_for_each(entry, &x86_boot_struct.memmap_entries, list) {
		info("  [mem %p - %p] %s\n",
			entry->start, entry->end - 1,
			entry->type == MEMMAP_USABLE_RAM ? "System RAM" :
			entry->type == MEMMAP_LOADER ? "Loader" :
			entry->type == MEMMAP_ACPI_DATA ? "ACPI Tables" :
			entry->type == MEMMAP_KERNEL ? "Kernel" :
			entry->type == MEMMAP_RESERVED ? "Reserved" :
			entry->type == MEMMAP_ACPI_NVS ? "ACPI NVS Memory" :
			"Unknown memory type");
	}

	for(;;)
		__builtin_ia32_pause();
}
