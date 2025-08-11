// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/smp.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ke/rwspinlock.h>
#include <Ke/smp.h>

unsigned int keProcessorCount = 1;

unsigned int kiCPUPresentBitmap[
	(CONFIG_MAX_NR_CPUS + (8 * sizeof(unsigned int)) - 1)
		/ (8 * sizeof(unsigned int))
] = { 1U };

unsigned int kiCPUOnlineBitmap[
	(CONFIG_MAX_NR_CPUS + (8 * sizeof(unsigned int)) - 1)
		/ (8 * sizeof(unsigned int))
] = { 1U };

/**
 * KiSmpAddCpu - increment keProcessorCount and return the new CPU number.
 * Returns zero if we run into CONFIG_MAX_NR_CPUS.
 *
 * KiSmpAddCpu should be called by the early HAL initialization code, most
 * likely somewhere in HalInitialize(), for every CPU that the HAL wishes to
 * later bring online.  KiSmpAddCpu should not be called for the BSP.
 */
unsigned int KiSmpAddCpu(void)
{
	if (keProcessorCount == CONFIG_MAX_NR_CPUS)
		return 0;

	return keProcessorCount++;
}

static KeDPCRWSpinlock cpus_rw_lock;

/**
 * KeReadLockCPUs - take the read lock on the CPU-present and CPU-online
 * bitmaps.
 */
void KeReadLockCPUs(void)
{
	cpus_rw_lock.read_lock();
}

/**
 * KeReadUnlockCPUs - release the read lock on the CPU-present and CPU-online
 * bitmaps.
 */
void KeReadUnlockCPUs(void)
{
	cpus_rw_lock.read_unlock();
}

/**
 * KeSetCPUPresent - mark a CPU as present in the system.
 * @cpu: CPU number
 */
void KeSetCPUPresent(unsigned int cpu)
{
	unsigned int word = cpu / (8 * sizeof(unsigned int));
	unsigned int bitidx = cpu % (8 * sizeof(unsigned int));
	unsigned int bit = 1U << bitidx;

	cpus_rw_lock.write_lock();
	kiCPUPresentBitmap[word] |= bit;
	cpus_rw_lock.write_unlock();
}

/**
 * KeSetCPUOnline - mark a CPU as online in the system.
 * @cpu: CPU number
 */
void KeSetCPUOnline(unsigned int cpu)
{
	unsigned int word = cpu / (8 * sizeof(unsigned int));
	unsigned int bitidx = cpu % (8 * sizeof(unsigned int));
	unsigned int bit = 1U << bitidx;

	cpus_rw_lock.write_lock();
	kiCPUOnlineBitmap[word] |= bit;
	cpus_rw_lock.write_unlock();
}

