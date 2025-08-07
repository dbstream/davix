// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/time/tsc.cc
 * Time Stamp Counter and HalNanosSinceBoot
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/time.h>
#include <davix/export.h>
#include "internal.h"

// Well... the TSC itself is a TODO.

bool HalUseHPETTimer = false;

unsigned long long HalNanosSinceBoot(void)
{
	if (HalUseHPETTimer) {
		return HalHPETNanosSinceBoot();
	}

	return 0; // fallback value
}
EXPORT_SYMBOL(HalNanosSinceBoot);

