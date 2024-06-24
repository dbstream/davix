/**
 * Detection of CPU features.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_FEATURES_H
#define _ASM_FEATURES_H 1

#include <asm/features_enum.h>

/**
 * Explanation of cpu_* fields:
 *	cpu_vendor	numeric ID of the CPU manufacturer, see CPU_VENDOR_*
 *	cpu_brand	CPU brand string, such as GenuineIntel or AuthenticAMD
 *	cpu_model	CPU model name
 */
extern unsigned int cpu_vendor;
extern char cpu_brand[];
extern char cpu_model[];

#define CPU_VENDOR_UNKNOWN	0
#define CPU_VENDOR_AMD		1
#define CPU_VENDOR_INTEL	2

extern unsigned int bsp_feature_array[];

static inline int
bsp_has (unsigned int feature)
{
	unsigned int feature_word = feature >> 5;
	unsigned int feature_bit = 1U << (feature & 31);
	return (bsp_feature_array[feature_word] & feature_bit) ? 1 : 0;
}

extern void
bsp_features_init (void);

#endif /* _ASM_FEATURES_H */
