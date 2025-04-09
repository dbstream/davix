/**
 * CPU feature enumeration.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>

enum {
	CPU_VENDOR_UNKNOWN = 0,
	CPU_VENDOR_AMD,
	CPU_VENDOR_INTEL
};

extern int cpu_vendor;
extern char cpu_brand_string[];
extern char cpu_model_string[];

extern uintptr_t x86_max_phys_addr;

extern uint32_t bsp_feature_array[];

extern uint64_t __cr0_state;
extern uint64_t __cr4_state;
extern uint64_t __efer_state;

void
cpufeature_init (void);

enum {
	/** Feature word 0: CPUID 01h ecx  */
	FEATURE_X2APIC			= 32 * 0	+ 21,

	/** Feature word 1: CPUID 01h edx  */
	FEATURE_TSC			= 32 * 1	+ 4,
	FEATURE_PAT			= 32 * 1	+ 16,

	/** Feature word 2: CPUID 07h ebx  */
	FEATURE_RDSEED			= 32 * 2	+ 18,

	/** Feature word 3: CPUID 07h ecx  */
	FEATURE_LA57			= 32 * 3	+ 16,

	/** Feature word 4: CPUID extended 01h edx  */
	FEATURE_NX			= 32 * 4	+ 20,
	FEATURE_PDPE1GB			= 32 * 4	+ 26,

	/** Feature word 5: CPUID extended 07h edx  */
	FEATURE_TSCINV			= 32 * 5	+ 8,

	FEATURE_MAX			= 32 * 6
};

static inline bool
has_feature (unsigned int feature)
{
	if (feature >= FEATURE_MAX)
		return false;

	unsigned int feature_word = feature >> 5;
	unsigned int feature_bit = 1U << (feature & 31);

	return (bsp_feature_array[feature_word] & feature_bit) ? 1 : 0;
}
