/**
 * x86-specific ELF functionality.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _ASM_ELF_H
#define _ASM_ELF_H 1

#include <davix/stdbool.h>

#define EM_X86_64	62

#define elf_is_supported_machine(e_machine) ((e_machine) == EM_X86_64)
#define elf_is_suppported_data(e_data) ((e_data) == ELFDATA2LSB)

#define ARCH_ELF_HEADER <davix/elf64.h>

#define ARCH_STACK_GROWS_DOWN 1

#endif /* _ASM_ELF_H */
