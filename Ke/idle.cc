// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/idle.cc
 * CPU idle loop.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/interrupt.h>
#include <Ke/idle.h>

void KeCPUIdleLoop(void)
{
	for (;;) {
		HalWaitForInterrupt();
	}
}

