/* SPDX-License-Identifier: MIT */
#ifndef __MULTIBOOT_H
#define __MULTIBOOT_H

#ifdef __ASSEMBLER__

#define MB2_MAGIC 0xe85250d6
#define MB2_LOADER_MAGIC 0x36d76289
#define MB2_ARCH_X86 0
#define MB2_CHECKSUM(hdrlen) (-(MB2_MAGIC + MB2_ARCH_X86 + (hdrlen)))
#define MB2_TAG_END 0
#define MB2_TAG_INFO_REQ 1

#else

#include <davix/types.h>

struct packed mb2_info {
	u32 size;
	u32 reserved;
};

struct packed mb2_tag_hdr {
	u32 type;
	u32 size;
};

#define MB2_TAG_END 0
#define MB2_TAG_MODULE 3
#define MB2_TAG_MEMMAP 6
#define MB2_TAG_RSDP 14
#define MB2_TAG_XSDP 15

struct packed mb2_module {
	struct mb2_tag_hdr hdr;
	u32 mod_start;
	u32 mod_end;
	char string[];
};

struct packed mb2_memmap {
	struct mb2_tag_hdr hdr;
	u32 entry_size;
	u32 revision;
	char entries[];
};

struct packed mb2_memmap_entry {
	u64 addr;
	u64 size;
	u32 type;
	u32 reserved;
};

#define MB2_MEMMAP_RAM 1	/* Normal System RAM */
#define MB2_MEMMAP_RECLAIM 3	/* ACPI Reclaimable Memory */
#define MB2_MEMMAP_ACPI_NVS 4	/* ACPI NVS memory, requires special handling */
#define MB2_MEMMAP_BADRAM 5	/* Defective System RAM */

struct packed mb2_rsdp {
	struct mb2_tag_hdr hdr;
	char ac_sig[8];
	u8 ac_chksum;
	char ac_OEMID[6];
	u8 ac_rev;
	u32 ac_pRSDT;
	u32 ac_len;
	u64 ac_pXSDT;
	u8 ac_xchksum;
	u8 ac_reserved[3];
};

static inline struct mb2_tag_hdr *mb2_first(struct mb2_info *info)
{
	return (struct mb2_tag_hdr *) ((unsigned long) info + 8);
}

static inline struct mb2_tag_hdr *mb2_next(struct mb2_tag_hdr *tag)
{
	return (struct mb2_tag_hdr *)
		(((unsigned long) tag + tag->size + 7) & ~7);
}

#define for_each_multiboot_tag(tag, info) \
	for(struct mb2_tag_hdr *tag = mb2_first(info); \
		(unsigned long) tag + 8 <= (unsigned long) (info) \
			+ (info)->size && (unsigned long) tag + tag->size \
			<= (unsigned long) (info) + (info)->size \
			&& tag->type != MB2_TAG_END; \
		tag = mb2_next(tag))

#endif
#endif
