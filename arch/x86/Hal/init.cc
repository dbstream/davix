// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/init.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ki/start_kernel.h>
#include "time/internal.h"

void HalInitialize(void)
{
	HalInitializeHPET();
}

