// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/asm/cpufeature.h
 * CPU features and control register state.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <stdint.h>

extern char cpu_brand_string[];
extern char cpu_model_string[];

extern uintptr_t x86_max_phys_addr;

extern uint32_t cpu_feature_array[];

extern uint64_t __cr0_state;
extern uint64_t __cr4_state;
extern uint64_t __efer_state;

void cpufeature_init(void);

enum : unsigned int {
	/* Feature word 0: CPUID 01h ecx */
	CPUFEATURE_X2APIC			= 32 * 0	+ 21,

	/* Feature word 1: CPUID 01h edx */
	CPUFEATURE_TSC				= 32 * 1	+ 4,
	CPUFEATURE_PAT				= 32 * 1	+ 16,

	/** Feature word 2: CPUID 07h ebx */
	CPUFEATURE_RDSEED			= 32 * 2	+ 18,

	/** Feature word 3: CPUID 07h ecx */
	CPUFEATURE_LA57				= 32 * 3	+ 16,

	/** Feature word 4: CPUID extended 01h edx */
	CPUFEATURE_NX				= 32 * 4	+ 20,
	CPUFEATURE_PDPE1GB			= 32 * 4	+ 26,

	/** Feature word 5: CPUID extended 07h edx */
	CPUFEATURE_TSCINV			= 32 * 5	+ 8,

	CPUFEATURE_MAX				= 32 * 6
};

static inline bool CPUFeature(unsigned int feature)
{
	if (feature >= CPUFEATURE_MAX)
		return false;

	unsigned int word_index = feature >> 5;
	unsigned int bit = 1U << (feature & 31);

	return (cpu_feature_array[word_index] & bit) ? true : false;
}

#define CPUFeature(x) CPUFeature(CPUFEATURE_##x)

