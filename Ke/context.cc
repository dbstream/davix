// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/context.cc
 * KiProcessorContext and dispatch routines.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/percpu.h>
#include <Ke/context.h>
#include <davix/bug.h>
#include <davix/export.h>

DEFINE_PERCPU(KiProcessorContext, kiProcessorContext);

HAL_PERCPU_CALLBACK(cpu)
{
	// Skip the BSP.
	if (cpu == 0)
		return;

	KiProcessorContext *ctx = HalPtrPerCPU(kiProcessorContext, cpu);

	ctx->noio_counter = 0;
	ctx->preemption_counter = KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	ctx->dpc_counter = KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	ctx->irq_counter = KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
}

void KeDispatchPendingPreemption(void)
{
	BUG(); // Not yet implemented.
}
EXPORT_SYMBOL(KeDispatchPendingPreemption)

void KeDispatchPendingDPCs(void)
{
	BUG(); // Not yet implemented.
}
EXPORT_SYMBOL(KeDispatchPendingDPCs)

void KeDispatchPendingIRQs(void)
{
	BUG(); // Not yet implemented.
}
EXPORT_SYMBOL(KeDispatchPendingIRQs)

