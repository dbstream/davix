// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/irq/entry.cc
 * Hal IRQ entry glue.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/interrupt.h>
#include <Hal/irq_vectors.h>
#include <Ke/context.h>
#include <Ke/irq.h>
#include <Ke/log.h>
#include <asm/entry.h>

extern "C" void HalHandleIRQVectorFromUserspace(entry_regs *regs)
{
	unsigned int vector = regs->error_code;
	KeEnterIRQContextFromUserspace();

	KeHandleInterruptVector(vector);

	KeExitIRQContextToUserspace();
}

extern "C" void HalHandleIRQVectorFromKernel(entry_regs *regs)
{
	unsigned int vector = regs->error_code;
	if (!KeEnterIRQContextFromKernel(vector)) {
		/* return with interrupts disabled */
		regs->rflags &= ~(1UL << 9);
		return;
	}

	KeHandleInterruptVector(vector);

	KeExitIRQContextToKernel();
}

void HalHandleSysvec(unsigned int vector)
{
	if (vector == IRQ_VECTOR_APIC_TIMER) {
		/*
		 * Local APIC Timer interrupt - HOT HOT HOT!
		 */
		[[likely]];
		HalAcknowledgeInterrupt();
		KeHandleLocalTimerInterrupt();
		return;
	}
}

