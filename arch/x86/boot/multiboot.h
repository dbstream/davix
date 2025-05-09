/**
 * Definitions for multiboot2.
 * Copyright (C) 2025-present  dbstream
 *
 * This file is based on the multiboot2 protocol specification, available at:
 *	<https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html>
 */
#pragma once

#ifdef __ASSEMBLER__
#	define MB2_MAGIC		0xe85250d6
#	define MB2_LOADER_MAGIC		0x36d76289
#	define MB2_ARCH_X86		0
#	define MB2_CHECKSUM(hdrlen)	(-(MB2_MAGIC + MB2_ARCH_X86 + (hdrlen)))
#	define MB2_TAG_END		0
#	define MB2_TAG_INFO_REQ		1
#	define MB2_TAG_LOAD_ADDR	2
#	define MB2_TAG_ENTRY_ADDR	3
#	define MB2_TAG_FRAMEBUFFER	5
#	define MB2_TAG_RELOCATABLE	10
#else /** __ASSEMBLER__  */

#include <stdint.h>

struct [[gnu::packed]] multiboot_params {
	uint32_t size;
	uint32_t reserved;
};

struct [[gnu::packed]] multiboot_tag {
	uint32_t type;
	uint32_t size;
};

#define MB2_TAG_END		0
#define MB2_TAG_CMDLINE		1
#define MB2_TAG_MODULE		3
#define MB2_TAG_MEMMAP		6
#define MB2_TAG_FRAMEBUFFER	8
#define MB2_TAG_RSDPv1		14
#define MB2_TAG_RSDPv2		15
#define MB2_TAG_EFI_MEMMAP	17

struct [[gnu::packed]] multiboot_string {
	uint32_t	type;
	uint32_t	size;
	char		value[];
};

struct [[gnu::packed]] multiboot_module {
	uint32_t	type;
	uint32_t	size;
	uint32_t	mod_start;
	uint32_t	mod_end;
	char		argument[];
};

struct [[gnu::packed]] multiboot_memmap {
	uint32_t	type;
	uint32_t	size;
	uint32_t	entry_size;
	uint32_t	entry_version;
};

#define MB2_MEMMAP_USABLE	1
#define MB2_MEMMAP_ACPI_RECLAIM	3
#define MB2_MEMMAP_ACPI_NVS	4
#define MB2_MEMMAP_DEFECTIVE	5

struct [[gnu::packed]] multiboot_memmap_entry {
	uint64_t	start;
	uint64_t	size;
	uint32_t	type;
	uint32_t	reserved;
};

#define MB2_FRAMEBUFFER_PALETTE	0
#define MB2_FRAMEBUFFER_COLOR	1
#define MB2_FRAMEBUFFER_TEXT	2

struct [[gnu::packed]] multiboot_framebuffer {
	uint32_t	type;
	uint32_t	size;
	uint64_t	framebuffer_addr;
	uint32_t	pitch;
	uint32_t	width;
	uint32_t	height;
	uint8_t		bpp;
	uint8_t		framebuffer_type;
	uint16_t	reserved;
	union [[gnu::packed]] {
		struct [[gnu::packed]] {
			uint32_t	num_palette_colors;
			struct __attribute__ ((packed)) {
				uint8_t r, g, b;
			} palette_colors[];
		};
		struct [[gnu::packed]] {
			uint8_t		red_shift;
			uint8_t		red_bits;
			uint8_t		green_shift;
			uint8_t		green_bits;
			uint8_t		blue_shift;
			uint8_t		blue_bits;
		};
	};
};

#endif /** !__ASSEMBLER__  */
