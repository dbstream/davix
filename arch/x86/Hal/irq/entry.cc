// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/irq/entry.cc
 * Hal IRQ entry glue.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/interrupt.h>
#include <Hal/irq_vectors.h>
#include <Hal/jiffies.h>
#include <Hal/smp.h>
#include <Hal/tlb.h>
#include <Ke/context.h>
#include <Ke/irq.h>
#include <Ke/log.h>
#include <asm/entry.h>
#include <davix/atomic.h>

unsigned long long jiffies;

extern "C" void HalHandleIRQVectorFromUserspace(entry_regs *regs)
{
	unsigned int vector = regs->error_code;

	if (vector == IRQ_VECTOR_APIC_TIMER && HalCurrentProcessor() == 0) {
		atomic_store_relaxed(&jiffies, jiffies + 1);
	}

	KeEnterIRQContextFromUserspace();

	KeHandleInterruptVector(vector);

	KeExitIRQContextToUserspace();
}

extern "C" void HalHandleIRQVectorFromKernel(entry_regs *regs)
{
	unsigned int vector = regs->error_code;

	if (vector == IRQ_VECTOR_APIC_TIMER && HalCurrentProcessor() == 0) {
		atomic_store_relaxed(&jiffies, jiffies + 1);
	}

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

	switch(vector) {
	case IRQ_VECTOR_TLBFLUSH:
		HalAcknowledgeInterrupt();
		HalHandleTLBFlushIPI();
		break;
	case IRQ_VECTOR_KERNEL_PANIC:
		[[unlikely]];
		KePanic("IRQ_VECTOR_KERNEL_PANIC was invoked on CPU%u!",
			HalCurrentProcessor()
		);
		break;
	case IRQ_VECTOR_RESCHEDULE:
		HalAcknowledgeInterrupt();
		KeHandleRescheduleIPI();
		break;
	case IRQ_VECTOR_SCHED_TIMER_DIRTY:
		HalAcknowledgeInterrupt();
		KeHandleSchedTimerRecalcIPI();
		break;
	}
}

