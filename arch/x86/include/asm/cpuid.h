/**
 * Support for the CPUID instruction.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_CPUID_H
#define _ASM_CPUID_H 1

#include <davix/stdint.h>

static inline void
__cpuid (uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
	asm volatile (
		"cpuid"
		: "+a" (*a), "=b" (*b), "+c" (*c), "=d" (*d)
		:: "memory"
	);
}

static inline void
cpuid (uint32_t leaf,
	uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
	*a = leaf;
	__cpuid (a, b, c, d);
}

static inline void
cpuid_subleaf (uint32_t leaf, uint32_t subleaf,
	uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
	*a = leaf;
	*c = subleaf;
	__cpuid (a, b, c, d);
}

#endif /* _ASM_CPUID_H */
