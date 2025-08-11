// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/smp.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

extern unsigned int keProcessorCount;

extern unsigned int kiCPUPresentBitmap[];
extern unsigned int kiCPUOnlineBitmap[];

/**
 * KeCPUPresent - test if a CPU is present in the system.
 * @cpu: CPU number
 *
 * This function must be called with either the hotplug semaphore or the CPU
 * bitmaps lock held.
 */
static inline bool KeCPUPresent(unsigned int cpu)
{
	if (cpu >= CONFIG_MAX_NR_CPUS)
		return false;

	if (cpu >= keProcessorCount)
		return false;

	unsigned int word = cpu / (8 * sizeof(unsigned int));
	unsigned int bitidx = cpu % (8 * sizeof(unsigned int));
	unsigned int bit = 1U << bitidx;

	return (kiCPUPresentBitmap[word] & bit) ? true : false;
}

/**
 * KeCPUOnline - test if a CPU is online.
 * @cpu: CPU number
 *
 * This function must be called with either the hotplug semaphore or the CPU
 * bitmaps lock held.
 */
static inline bool KeCPUOnline(unsigned int cpu)
{
	if (cpu >= CONFIG_MAX_NR_CPUS)
		return false;

	if (cpu >= keProcessorCount)
		return false;

	unsigned int word = cpu / (8 * sizeof(unsigned int));
	unsigned int bitidx = cpu % (8 * sizeof(unsigned int));
	unsigned int bit = 1U << bitidx;

	return (kiCPUOnlineBitmap[word] & bit) ? true : false;
}

unsigned int KiSmpAddCpu(void);

void KeReadLockCPUs(void);

void KeReadUnlockCPUs(void);

void KeSetCPUPresent(unsigned int cpu);

void KeSetCPUOnline(unsigned int cpu);

