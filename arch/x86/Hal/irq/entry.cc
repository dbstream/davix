// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/irq/entry.cc
 * Hal IRQ entry glue.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/interrupt.h>
#include <Ke/log.h>
#include <asm/entry.h>

extern "C" void HalHandleIRQVectorFromUserspace(entry_regs *regs)
{
	(void) regs;
}

extern "C" void HalHandleIRQVectorFromKernel(entry_regs *regs)
{
	(void) regs;
}

