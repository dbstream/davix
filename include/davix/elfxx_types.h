/**
 * Common part of elf64.h and elf32.h.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_ELFXX_TYPES_H
#define _DAVIX_ELFXX_TYPES_H 1

#ifdef __LINTER__
#define ELFBITS 64
#endif

#ifndef ELFBITS
#error Define ELFBITS before including elfxx_types.h.
#endif

#include <davix/elf_types.h>

/* Evil macro hacks to splice multiple tokens into one. */
#define __ELFXX__CAT2(a,b) a##b
#define __ELFXX_CAT2(a,b) __ELFXX__CAT2(a,b)
#define __ELFXX__CAT3(a,b,c) a##b##c
#define __ELFXX_CAT3(a,b,c) __ELFXX__CAT3(a,b,c)

#define ElfXX_Half __ELFXX_CAT3(Elf, ELFBITS, _Half)
#define ElfXX_Word __ELFXX_CAT3(Elf, ELFBITS, _Word)
#define ElfXX_Sword __ELFXX_CAT3(Elf, ELFBITS, _Sword)
#define ElfXX_Xword __ELFXX_CAT3(Elf, ELFBITS, _Xword)
#define ElfXX_Sxword __ELFXX_CAT3(Elf, ELFBITS, _Sxword)
#define ElfXX_Addr __ELFXX_CAT3(Elf, ELFBITS, _Addr)
#define ElfXX_Off __ELFXX_CAT3(Elf, ELFBITS, _Off)
#define ElfXX_Section __ELFXX_CAT3(Elf, ELFBITS, _Section)
#define ElfXX_Versym __ELFXX_CAT3(Elf, ELFBITS, _Versym)
#define ElfXX_Ehdr __ELFXX_CAT3(Elf, ELFBITS, _Ehdr)
#define ElfXX_Shdr __ELFXX_CAT3(Elf, ELFBITS, _Shdr)
#define ElfXX_Sym __ELFXX_CAT3(Elf, ELFBITS, _Sym)
#define ElfXX_Rel __ELFXX_CAT3(Elf, ELFBITS, _Rel)
#define ElfXX_Rela __ELFXX_CAT3(Elf, ELFBITS, _Rela)
#define ELFXX_R_SYM __ELFXX_CAT3(ELF, ELFBITS, _R_SYM)
#define ELFXX_R_TYPE __ELFXX_CAT3(ELF, ELFBITS, _R_TYPE)
#define ELFXX_R_INFO __ELFXX_CAT3(ELF, ELFBITS, _R_INFO)
#define ElfXX_Phdr __ELFXX_CAT3(Elf, ELFBITS, _Phdr)
#define ElfXX_Dyn __ELFXX_CAT3(Elf, ELFBITS, _Dyn)
#define ELFCLASSXX __ELFXX_CAT2(ELFCLASS, ELFBITS)

#endif /* _DAVIX_ELFXX_TYPES_H */
