/* SPDX-License-Identifier: MIT */
#include <davix/printk.h>
#include <davix/list.h>
#include <asm/boot.h>
#include <asm/page.h>
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

unsigned long HHDM_OFFSET;

void x86_start_kernel(void);
void x86_start_kernel(void)
{
	HHDM_OFFSET = x86_boot_struct.l5_paging_enable ? 0xff00000000000000UL : 0xffff800000000000UL;

	x86_uart_init();

	info("x86/kernel: Booted with protocol version %u.%u.%u by \"%s\". LA57: %s\n",
		BOOTPROTOCOL_VERSION_MAJOR(x86_boot_struct.protocol_version),
		BOOTPROTOCOL_VERSION_MINOR(x86_boot_struct.protocol_version),
		BOOTPROTOCOL_VERSION_PATCH(x86_boot_struct.protocol_version),
		x86_boot_struct.bootloader_name,
		x86_boot_struct.l5_paging_enable ? "on" : "off");

	debug("x86/kernel: HHDM_OFFSET = %p\n", HHDM_OFFSET);

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

#if 0
		/*
		 * Useful when messing with page tables.
		 */
		if(entry->type == MEMMAP_USABLE_RAM) {
			unsigned long start = align_up(entry->start, PAGE_SIZE);
			unsigned long end = align_down(entry->end, PAGE_SIZE);
			for(; start < end; start += PAGE_SIZE) {
				*(volatile unsigned long *) phys_to_virt(start)
					= *(volatile unsigned long *)
						phys_to_virt(start) ^ start;
			}
		}
#endif
	}

	info("x86/kernel: Done.\n");
	for(;;)
		__builtin_ia32_pause();
}
