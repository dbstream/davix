// SPDX-License-Identifier: GPL-3.0
/*
 * File: boot/cpu_feat.cc
 * CPU Features Initialization.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/pteflags.h>
#include <asm/cpufeature.h>
#include <asm/creg_access.h>
#include <asm/creg_bits.h>
#include <asm/msraccess.h>
#include <asm/msr_bits.h>
#include <asm/pg_bits.h>
#include <davix/export.h>

char cpu_brand_string[13] = "(unknown)";
char cpu_model_string[49] = "(unknown)";

uintptr_t x86_max_phys_addr = 0x100000000UL;

uint64_t x86_nx_bit = 0;

uint32_t cpu_feature_array[CPUFEATURE_MAX / 32];

uint64_t __cr0_state;
uint64_t __cr4_state;
uint64_t __efer_state;

PTEFLAGS PTEFLAGS_READONLY;
PTEFLAGS PTEFLAGS_READWRITE;
PTEFLAGS PTEFLAGS_READEXEC;
EXPORT_SYMBOL(PTEFLAGS_READONLY)
EXPORT_SYMBOL(PTEFLAGS_READWRITE)
EXPORT_SYMBOL(PTEFLAGS_READEXEC)

static inline void cpuid(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d)
{
	asm volatile ("cpuid" : "+a" (a), "=b" (b), "+c" (c), "=d" (d) :: "memory" );
}

static int clamp(int min, int x, int max)
{
	if (min > x) x = min;
	if (max < x) x = max;
	return x;
}

void cpufeature_init(void)
{
	uint32_t a, b, c, d;
#define cpuid cpuid(a, b, c, d)
	a = 0;
	c = 0;
	cpuid;

	cpu_brand_string[0] = b;
	cpu_brand_string[1] = b >> 8;
	cpu_brand_string[2] = b >> 16;
	cpu_brand_string[3] = b >> 24;
	cpu_brand_string[4] = d;
	cpu_brand_string[5] = d >> 8;
	cpu_brand_string[6] = d >> 16;
	cpu_brand_string[7] = d >> 24;
	cpu_brand_string[8] = c;
	cpu_brand_string[9] = c >> 8;
	cpu_brand_string[10] = c >> 16;
	cpu_brand_string[11] = c >> 24;
	cpu_brand_string[12] = 0;

	uint32_t maxleaf = a;
	if (maxleaf >= 0x07U) {
		a = 0x07U;
		c = 0;
		cpuid;
		cpu_feature_array[2] = b;
		cpu_feature_array[3] = c;
	}
	if (maxleaf >= 0x01U) {
		a = 0x01U;
		c = 0;
		cpuid;
		cpu_feature_array[0] = c;
		cpu_feature_array[1] = d;
	}

	a = 0x80000000;
	c = 0;
	cpuid;

	maxleaf = a;
	if (maxleaf >= 0x80000008U) {
		a = 0x80000008U;
		c = 0;
		cpuid;
		x86_max_phys_addr = 1UL << clamp(32, a & 0xffU, 52);
	}
	if (maxleaf >= 0x80000007U) {
		a = 0x80000007U;
		c = 0;
		cpuid;
		cpu_feature_array[5] = d;
	}
	if (maxleaf >= 0x80000004U) {
		for (int i = 0; i < 3; i++) {
			a = 0x80000002U + i;
			c = 0;
			cpuid;
			cpu_model_string[16 * i + 0] = a;
			cpu_model_string[16 * i + 1] = a >> 8;
			cpu_model_string[16 * i + 2] = a >> 16;
			cpu_model_string[16 * i + 3] = a >> 24;
			cpu_model_string[16 * i + 4] = b;
			cpu_model_string[16 * i + 5] = b >> 8;
			cpu_model_string[16 * i + 6] = b >> 16;
			cpu_model_string[16 * i + 7] = b >> 24;
			cpu_model_string[16 * i + 8] = c;
			cpu_model_string[16 * i + 9] = c >> 8;
			cpu_model_string[16 * i + 10] = c >> 16;
			cpu_model_string[16 * i + 11] = c >> 24;
			cpu_model_string[16 * i + 12] = d;
			cpu_model_string[16 * i + 13] = d >> 8;
			cpu_model_string[16 * i + 14] = d >> 16;
			cpu_model_string[16 * i + 15] = d >> 24;
		}
		cpu_model_string[48] = 0;
	}
	if (maxleaf >= 0x80000001U) {
		a = 0x80000001U;
		c = 0;
		cpuid;
		cpu_feature_array[4] = d;
	}

	__cr0_state = read_cr0();
	__cr4_state = read_cr4();
	__efer_state = _EFER_LME;
	if (CPUFeature(NX)) {
		x86_nx_bit = __PG_NX;
		__efer_state |= _EFER_NXE;
	}
	write_msr(MSR_EFER, __efer_state);

	if (CPUFeature(PAT)) {
		/*
		 * Other CPUs will check CPUFeature(PAT) and install the
		 * correct table into MSR_PAT early in the process of being
		 * brought online.
		 */
		write_msr(MSR_PAT, 0x100070406UL);
	}

	PTEFLAGS_READONLY = HalMakePTEFlags(PAGE_WRITEBACK, 1, 0, 0);
	PTEFLAGS_READWRITE = HalMakePTEFlags(PAGE_WRITEBACK, 1, 1, 0);
	PTEFLAGS_READEXEC = HalMakePTEFlags(PAGE_WRITEBACK, 1, 0, 1);
}

