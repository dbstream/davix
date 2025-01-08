/**
 * Header for ELF64
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_ELF64_H
#define _DAVIX_ELF64_H 1

#ifdef _DAVIX_ELF32_H
#error It is forbidden to include both elf64.h and elf32.h from the same file.
#endif /* _DAVIX_ELF32_H */

#define ELFBITS 64
#include "elfxx_types.h"

#endif /* _DAVIX_ELF64_H */
