// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/irq/handle.cc
 * Interrupt handling.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/interrupt.h>
#include <Hal/irq_vectors.h>
#include <Ke/irq.h>
#include <Ke/log.h>

/**
 * KeHandleInterruptVector - handle an interrupt.
 * @vector: interrupt vector number
 *
 * This is the main code path for ALL hardware interrupts;  everything passes
 * through here.  As such, it is a very hot codepath.
 */
void KeHandleInterruptVector(unsigned int vector)
{
	if (vector >= IRQ_VECTOR_SYSTEM_FIRST) {
		HalHandleSysvec(vector);
		return;
	}

	KePrintf("warning: KeHandleInterruptVector(%u): unimplemented; not sending EOI\n",
			vector);
}

