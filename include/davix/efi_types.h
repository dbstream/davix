/** 
 * UEFI type definitions, mainly memory map.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>

#define EFI_MEMORY_RESERVED		0
#define EFI_MEMORY_LOADER_CODE		1
#define EFI_MEMORY_LOADER_DATA		2
#define EFI_MEMORY_BOOT_SERVICES_CODE	3
#define EFI_MEMORY_BOOT_SERVICES_DATA	4
#define EFI_MEMORY_RT_SERVICES_CODE	5
#define EFI_MEMORY_RT_SERVICES_DATA	6
#define EFI_MEMORY_CONVENTIONAL_RAM	7
#define EFI_MEMORY_UNUSABLE		8
#define EFI_MEMORY_ACPI_RECLAIM		9
#define EFI_MEMORY_ACPI_NVS		10
#define EFI_MEMORY_MMIO			11
#define EFI_MEMORY_MMIO_PORT_SPACE	12
#define EFI_MEMORY_PAL_CODE		13
#define EFI_MEMORY_PERSISTENT_RAM	14
#define EFI_MEMORY_UNACCEPTED		15

#define _EFI_MEMORY_ATTRIBUTE(n)	(UINT64_C(1) << n)
#define EFI_MEMORY_UC			_EFI_MEMORY_ATTRIBUTE(0)
#define EFI_MEMORY_WC			_EFI_MEMORY_ATTRIBUTE(1)
#define EFI_MEMORY_WT			_EFI_MEMORY_ATTRIBUTE(2)
#define EFI_MEMORY_WB			_EFI_MEMORY_ATTRIBUTE(3)
#define EFI_MEMORY_UCE			_EFI_MEMORY_ATTRIBUTE(4)
#define EFI_MEMORY_WP			_EFI_MEMORY_ATTRIBUTE(12)
#define EFI_MEMORY_RP			_EFI_MEMORY_ATTRIBUTE(13)
#define EFI_MEMORY_XP			_EFI_MEMORY_ATTRIBUTE(14)
#define EFI_MEMORY_NV			_EFI_MEMORY_ATTRIBUTE(15)
#define EFI_MEMORY_MORE_RELIABLE	_EFI_MEMORY_ATTRIBUTE(16)
#define EFI_MEMORY_RO			_EFI_MEMORY_ATTRIBUTE(17)
#define EFI_MEMORY_SP			_EFI_MEMORY_ATTRIBUTE(18)
#define EFI_MEMORY_CPU_CRYPTO		_EFI_MEMORY_ATTRIBUTE(19)
#define EFI_MEMORY_RUNTIME		_EFI_MEMORY_ATTRIBUTE(63)
#define EFI_MEMORY_ISA_VALID		_EFI_MEMORY_ATTRIBUTE(62)

struct [[gnu::packed]] efi_memory_descriptor {
	uint32_t type;
	uint32_t reserved;
	uint64_t phys_start;
	uint64_t virt_start;
	uint64_t num_pages;
	uint64_t attribute;
};
