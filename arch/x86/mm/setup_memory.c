/* SPDX-License-Identifier: MIT */

#include <davix/printk.h>
#include <davix/list.h>
#include <asm/page.h>
#include <asm/boot.h>

unsigned long HHDM_OFFSET;

void x86_setup_memory(void);
void x86_setup_memory(void)
{
	HHDM_OFFSET = x86_boot_struct.l5_paging_enable
		? 0xff00000000000000UL : 0xffff800000000000UL;

	struct memmap_entry *entry;
	info("x86/mm: Memory map:\n");
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
}
