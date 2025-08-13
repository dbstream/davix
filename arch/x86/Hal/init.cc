// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/init.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/smpboot.h>
#include <Ke/log.h>
#include <Ke/smp.h>
#include <Ki/start_kernel.h>
#include "time/internal.h"
#include "irq/internal.h"

void HalInitialize(void)
{
	HalInitializeHPET();
	HalInitializeTSC();
	HalInitializeIRQSubsystem();

	HalPrepareSMPBringup();
}

