// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/smp.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ke/smp.h>

unsigned int keProcessorCount = 1;

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

