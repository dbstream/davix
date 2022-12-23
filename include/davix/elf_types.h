/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_ELF_TYPES_H
#define __DAVIX_ELF_TYPES_H

#include <davix/types.h>

struct packed elfhdr_common {
	char mag[4];	/* ELF_MAGIC */
	u8 elfclass;	/* Elf32 or Elf64? */
	u8 data;	/* Endianness */
	u8 elfversion;	/* ELF specification version (ignored) */
	u8 osabi;	/* Target OS ABI (ignored) */
	u8 abi_ver;	/* Target OS ABI version (ignored) */
	u8 pad[7];

	/*
	 * Interpret the following fields with respect to endianness.
	 */
	u16 type;	/* File type */
	u16 machine;	/* Target architecture */
	u32 version;	/* (ignored) */
};

struct packed elf64_hdr {
	struct elfhdr_common hdr;
	u64 entry;	/* Entry point */
	u64 phoff;	/* Program header table offset */
	u64 shoff;	/* Section header table offset (ignored) */
	u32 flags;	/* Various ELF flags, specific to architecture */
	u16 ehsize;	/* ELF header size (ignored) */
	u16 phentsize;	/* The size of a program header */
	u16 phnum;	/* The number of program headers */
	u16 shentsize;	/* The size of a section header (ignored) */
	u16 shnum;	/* The number of section headers (ignored) */
	u16 shstrndx;	/* The index of the section of section names */
};

#define ELF_MAGIC "\x7f\x45\x4c\x46"

#define ELF_CLASS_32BIT 1
#define ELF_CLASS_64BIT 2

#define ELF_ORDER_LITTLEENDIAN 1
#define ELF_ORDER_BIGENDIAN 2

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4

#define EM_AMD64 0x3e

struct packed elf64_phdr {
	u32 type;	/* Program header type */
	u32 flags;	/* Type-specific flags field */
	u64 offset;	/* Offset into file */
	u64 vaddr;	/* Virtual address */
	u64 paddr;	/* Physical address (used internally) */
	u64 filesz;	/* Amount of memory to read from the file */
	u64 memsz;	/* Amount of memory to allocate for the segment */
	u64 align;	/* Program header alignment */
};

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_TLS 7

#define PF_R 0x4
#define PF_W 0x2
#define PF_X 0x1

#endif /* __DAVIX_ELF_TYPES_H */
