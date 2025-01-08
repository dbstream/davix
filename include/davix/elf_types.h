/**
 * Type definitions for the ELF file format.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_ELF_TYPES_H
#define _DAVIX_ELF_TYPES_H 1

#include <davix/stdint.h>
#include <asm/elf.h>

typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Section;
typedef uint16_t Elf64_Versym;

typedef struct {
	unsigned char	e_ident[16];
	Elf64_Half	e_type;
	Elf64_Half	e_machine;
	Elf64_Word	e_version;
	Elf64_Addr	e_entry;
	Elf64_Off	e_phoff;
	Elf64_Off	e_shoff;
	Elf64_Word	e_flags;
	Elf64_Half	e_ehsize;
	Elf64_Half	e_phentsize;
	Elf64_Half	e_phnum;
	Elf64_Half	e_shentsize;
	Elf64_Half	e_shnum;
	Elf64_Half	e_shstrndx;
} Elf64_Ehdr;

#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EI_OSABI	7
#define EI_ABIVERSION	8

#define ELFMAG0		'\x7f'
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'

#define ELFCLASS32	1
#define ELFCLASS64	2

#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

#define EV_CURRENT	1

#define ELFOSABI_SYSV	0

#define ET_NONE		0U
#define ET_REL		1U
#define ET_EXEC		2U
#define ET_DYN		3U
#define ET_CORE		4U

#define SHN_UNDEF	0U
#define SHN_ABS		0xfff1U
#define SHN_COMMON	0xfff2U

typedef struct {
	Elf64_Word	sh_name;
	Elf64_Word	sh_type;
	Elf64_Xword	sh_flags;
	Elf64_Addr	sh_addr;
	Elf64_Off	sh_offset;
	Elf64_Xword	sh_size;
	Elf64_Word	sh_link;
	Elf64_Word	sh_info;
	Elf64_Xword	sh_addralign;
	Elf64_Xword	sh_entsize;
} Elf64_Shdr;

#define SHT_NULL	0U
#define SHT_PROGBITS	1U
#define SHT_SYMTAB	2U
#define SHT_STRTAB	3U
#define SHT_RELA	4U
#define SHT_HASH	5U
#define SHT_DYNAMIC	6U
#define SHT_NOTE	7U
#define SHT_NOBITS	8U
#define SHT_REL		9U
#define SHT_SHLIB	10U
#define SHT_DYNSYM	11U

#define SHF_WRITE	0x1U
#define SHF_ALLOC	0x2U
#define SHF_EXECINSTR	0x4U

typedef struct {
	Elf64_Word	st_name;
	unsigned char	st_info;
	unsigned char	st_other;
	Elf64_Half	st_shndx;
	Elf64_Addr	st_value;
	Elf64_Xword	st_size;
} Elf64_Sym;

#define STB_LOCAL	0U
#define STB_GLOBAL	1U
#define STB_WEAK	2U
#define STT_NOTYPE	0U
#define STT_OBJECT	1U
#define STT_FUNC	2U
#define STT_SECTION	3U
#define STT_FILE	4U

typedef struct {
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;
} Elf64_Rel;

typedef struct {
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;
	Elf64_Sxword	r_addend;
} Elf64_Rela;

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffffUL)
#define ELF64_R_INFO(s, t) (((s) << 32) | ((t) & 0xffffffffUL))

typedef struct {
	Elf64_Word	p_type;
	Elf64_Word	p_flags;
	Elf64_Off	p_offset;
	Elf64_Addr	p_vaddr;
	Elf64_Addr	p_paddr;
	Elf64_Xword	p_filesz;
	Elf64_Xword	p_memsz;
	Elf64_Xword	p_align;
} Elf64_Phdr;

#define PT_NULL		0U
#define PT_LOAD		1U
#define PT_DYNAMIC	2U
#define PT_INTERP	3U
#define PT_NOTE		4U
#define PT_SHLIB	5U
#define PT_PHDR		6U
#define PT_TLS		7U

#define PT_GNU_STACK		0x6474e551U

#define PF_X		0x1U
#define PF_W		0x2U
#define PF_R		0x4U

typedef struct {
	Elf64_Sxword		d_tag;
	union {
		Elf64_Xword	d_val;
		Elf64_Addr	d_ptr;
	} d_un;
} Elf64_Dyn;

#define DT_NULL		0U
#define DT_NEEDED	1U
#define DT_PLTRELSZ	2U
#define DT_PLTGOT	3U
#define DT_HASH		4U
#define DT_STRTAB	5U
#define DT_SYMTAB	6U
#define DT_RELA		7U
#define DT_RELASZ	8U
#define DT_RELAENT	9U
#define DT_STRSZ	10U
#define DT_SYMENT	11U
#define DT_INIT		12U
#define DT_FINI		13U
#define DT_SONAME	14U
#define DT_RPATH	15U
#define DT_SYMBOLIC	16U
#define DT_REL		17U
#define DT_RELSZ	18U
#define DT_RELENT	19U
#define DT_PLTREL	20U
#define DT_DEBUG	21U
#define DT_TEXTREL	22U
#define DT_JMPREL	23U
#define DT_BIND_NOW	24U
#define DT_INIT_ARRAY	25U
#define DT_FINI_ARRAY	26U
#define DT_INIT_ARRAYSZ	27U
#define DT_FINI_ARRAYSZ	28U

#endif /* _DAVIX_ELF_TYPES_H */
