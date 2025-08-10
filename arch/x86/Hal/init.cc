// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/init.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ki/start_kernel.h>
#include "time/internal.h"
#include "irq/internal.h"

void HalInitialize(void)
{
	HalInitializeHPET();
	HalInitializeTSC();
	HalInitializeIRQSubsystem();

	for (int i = 0; i < 1000000; i++) __builtin_ia32_pause();
}

