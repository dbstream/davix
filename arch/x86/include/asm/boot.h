/* SPDX-License-Identifier: MIT */
#ifndef __ASM_BOOT_H
#define __ASM_BOOT_H

#include <davix/types.h>

enum memmap_type {
	MEMMAP_USABLE_RAM = 1,		/* System RAM */
	MEMMAP_LOADER = 2,		/* Loader */
	MEMMAP_ACPI_DATA = 3,		/* ACPI tables and such */
	MEMMAP_KERNEL = 4,		/* Kernel and modules */
	MEMMAP_RESERVED = 5,		/* Reserved regions */
	MEMMAP_ACPI_NVS = 6,		/* ACPI NVS memory */
};

struct memmap_entry {
	unsigned long start;
	unsigned long end;
	struct list list;
	enum memmap_type type;
};

/*
 * x86 boot_struct, always located at 0xffffffff80000000.
 */
struct boot_struct {
	char bootstruct_magic[24];	/* "\177DAVIX_KERNEL_BOOT_INFO" */

	/*
	 * Protocol version.
	 *
	 * The bootloader will read this field to determine what boot protocol
	 * the kernel supports and then fill this in to indicate to the kernel
	 * what fields the bootloader supports.
	 */
	u64 protocol_version;

	union {
		char kernel[64];		/* Kernel name (and optionally version) */
		char bootloader_name[64];	/* Filled in by bootloader */
	};
	/*
	 * Memory map provided to the kernel.
	 * This must contain all reserved ranges, all usable RAM, and the
	 * kernel itself must be marked as MEMMAP_KERNEL in here.
	 *
	 * The entries may be stored in any place in virtual memory, so long as
	 * it is mapped when the kernel entry point is called (either in the
	 * HHDM or in lower-half memory which is not explicitly touched by the
	 * kernel until these entries have been consumed).
	 */
	struct list memmap_entries;

	/*
	 * Was the kernel booted with LA57?
	 */
	char l5_paging_enable;

	/*
	 * Are COM1 and COM2 initialized for us?
	 * (have they been used by the bootloader?)
	 */
	char com1_initialized;
	char com2_initialized;
};

#define BOOTSTRUCT_MAGIC "\177DAVIX_KERNEL_BOOT_INFO"

#define BOOTPROTOCOL_VERSION(major, minor, patch) \
	((((unsigned long) major) << 48) \
		| (((unsigned long) minor) << 32) \
		| (((unsigned long) patch)))

#define BOOTPROTOCOL_VERSION_MAJOR(version) ((version) >> 48)
#define BOOTPROTOCOL_VERSION_MINOR(version) (((version) >> 32) & 0xffff)
#define BOOTPROTOCOL_VERSION_PATCH(version) ((version) & 0xffffffff)

#define BOOTPROTOCOL_VERSION_1_0_0 BOOTPROTOCOL_VERSION(1, 0, 0)

#ifdef __KERNEL__

extern struct boot_struct x86_boot_struct section(".bootstruct");

#endif

#endif /* __ASM_BOOT_H */
