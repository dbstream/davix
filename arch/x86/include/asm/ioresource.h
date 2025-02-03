/**
 * SystemMemory and SystemIO ioresource ranges.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _ASM_IORESOURCE_H
#define _ASM_IORESOURCE_H 1

#include <asm/features.h>

static inline unsigned long
arch_iores_system_io_start (void)
{
	return 0;
}

static inline unsigned long
arch_iores_system_io_end (void)
{
	return 65536;
}

static inline unsigned long
arch_iores_system_memory_start (void)
{
	return 0;
}

static inline unsigned long
arch_iores_system_memory_end (void)
{
	return x86_max_phys_addr;
}

#endif /** _ASM_IORESOURCE_H  */

