// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/percpu.cc
 * HAL support routines for percpu variables.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/percpu.h>

namespace Hal {
	unsigned long percpu_offsets[1];
}

extern "C" char __percpu_callbacks_start[];
extern "C" char __percpu_callbacks_end[];

/**
 * HalInitializePerCPUVariables - call percpu variable initialization routines.
 * @cpu: CPU for which to call percpu variable initialization routines.
 */
void HalInitializePerCPUVariables(unsigned int cpu)
{
	auto start = (void (*const *)(unsigned int)) __percpu_callbacks_start;
	auto end = (void (*const *)(unsigned int)) __percpu_callbacks_end;
	asm("" : "+r"(start), "+r"(end));

	for (; start != end; start++)
		(*start)(cpu);
}

