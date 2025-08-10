// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/context.h
 * Execution context management.
 *
 * Copyright (C) 2025  dbstream
 *
 * This file contains the functions used to mask/unmask involuntary preemption,
 * DPCs, and IRQs.  Additionally, it contains functions that facilitate "No IO"
 * contexts, which allows to make some kernel subsystems return an error instead
 * of initiating I/O.
 */
#pragma once

#include <Hal/percpu.h>

enum : unsigned int {
	KI_CONTEXT_COUNTER_EVENT_NOT_PENDING = 1U << 31
};

struct KiProcessorContext {
	unsigned int preemption_counter;
	unsigned int dpc_counter;
	unsigned int irq_counter;
	unsigned int noio_counter;
};

extern KiProcessorContext kiProcessorContext;

static inline bool KiEnablePreemptionAndTest(void)
{
	return HalDecrementAndTestPerCPU(kiProcessorContext.preemption_counter);
}

static inline bool KiEnableDPCsAndTest(void)
{
	return HalDecrementAndTestPerCPU(kiProcessorContext.dpc_counter);
}

static inline bool KiEnableIRQsAndTest(void)
{
	return HalDecrementAndTestPerCPU(kiProcessorContext.irq_counter);
}

void KeDispatchPendingPreemption(void);

void KeDispatchPendingDPCs(void);

void KeDispatchPendingIRQs(void);

/**
 * KeDisablePreemption - disable involuntary thread preemption on the current
 * CPU.
 */
static inline void KeDisablePreemption(void)
{
	HalIncrementPerCPU(kiProcessorContext.preemption_counter);
}

/**
 * KeEnablePreemption - enable involuntary thread preemption on the current CPU.
 */
static inline void KeEnablePreemption(void)
{
	if (KiEnablePreemptionAndTest()) {
		[[unlikely]];
		KeDispatchPendingPreemption();
	}
}

/**
 * KePreemptionEnabled - check whether involuntary thread preemption is enabled
 * on the current CPU.
 * Returns true if involuntary thread preemption is enabled.
 */
static inline bool KePreemptionEnabled(void)
{
	unsigned int cnt = HalReadPerCPU(kiProcessorContext.preemption_counter);
	return (cnt & ~KI_CONTEXT_COUNTER_EVENT_NOT_PENDING) == 0;
}

/**
 * KePendingPreemption - check whether there is a pending preemption event on
 * the current CPU.
 * Returns true if there is a pending preemption event on the current CPU.
 */
static inline bool KePendingPreemption(void)
{
	unsigned int cnt = HalReadPerCPU(kiProcessorContext.preemption_counter);
	return (cnt & KI_CONTEXT_COUNTER_EVENT_NOT_PENDING) == 0;
}

/**
 * KeSetPendingPreemption - raise a preemption event on the current CPU.
 */
static inline void KeSetPendingPreemption(void)
{
	unsigned int cnt = HalReadPerCPU(kiProcessorContext.preemption_counter);
	cnt &= ~KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	HalWritePerCPU(kiProcessorContext.preemption_counter, cnt);
}

/**
 * KeDisableDPCs - disable deferred procedure calls (DPCs) on the current CPU.
 */
static inline void KeDisableDPCs(void)
{
	HalIncrementPerCPU(kiProcessorContext.dpc_counter);
}

/**
 * KeEnableDPCs - enable deferred procedure calls (DPCs) on the current CPU.
 */
static inline void KeEnableDPCs(void)
{
	if (KiEnableDPCsAndTest()) {
		[[unlikely]];
		KeDispatchPendingDPCs();
	}
}

/**
 * KeDPCsEnabled - check whether deferred procedure calls (DPCs) are enabled on
 * the current CPU.
 * Returns true if DPCs are enabled on the current CPU.
 */
static inline bool KeDPCsEnabled(void)
{
	unsigned int cnt = HalReadPerCPU(kiProcessorContext.dpc_counter);
	return (cnt & ~KI_CONTEXT_COUNTER_EVENT_NOT_PENDING) == 0;
}

/**
 * KePendingDPC - check whether there is a pending deferred procedure call (DPC)
 * on the current CPU.
 * Returns true if the current CPU has one or more pending DPCs to process.
 */
static inline bool KePendingDPC(void)
{
	unsigned int cnt = HalReadPerCPU(kiProcessorContext.dpc_counter);
	return (cnt & KI_CONTEXT_COUNTER_EVENT_NOT_PENDING) == 0;
}

/**
 * KeSetPendingDPC - raise a pending deferred procedure call (DPC) event on this
 * CPU.
 */
static inline void KeSetPendingDPC(void)
{
	unsigned int cnt = HalReadPerCPU(kiProcessorContext.dpc_counter);
	cnt &= ~KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	HalWritePerCPU(kiProcessorContext.dpc_counter, cnt);
}

/**
 * KeClearPendingDPC - clear the flag indicating pending deferred procedure
 * calls on this CPU.
 */
static inline void KeClearPendingDPC(void)
{
	unsigned int cnt = HalReadPerCPU(kiProcessorContext.dpc_counter);
	cnt |= KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	HalWritePerCPU(kiProcessorContext.dpc_counter, cnt);
}

/**
 * KeDisableIRQs - disable IRQ dispatching on the current CPU.
 */
static inline void KeDisableIRQs(void)
{
	HalIncrementPerCPU(kiProcessorContext.irq_counter);
}

/**
 * KeEnableIRQs - enable IRQ dispatching on the current CPU.
 */
static inline void KeEnableIRQs(void)
{
	if (KiEnableIRQsAndTest()) {
		[[unlikely]];
		KeDispatchPendingIRQs();
	}
}

/**
 * KeIRQsEnabled - check if IRQs are enabled on the current CPU.
 * Returns true if IRQs are enabled on the current CPU.
 */
static inline bool KeIRQsEnabled(void)
{
	unsigned int cnt = HalReadPerCPU(kiProcessorContext.irq_counter);
	return (cnt & ~KI_CONTEXT_COUNTER_EVENT_NOT_PENDING) == 0;
}

/**
 * KePendingIRQ - check if there is a pending IRQ to be dispatched on the
 * current CPU.
 * Returns true if there is a pending IRQ to be dispatched on the current CPU.
 */
static inline bool KePendingIRQ(void)
{
	unsigned int cnt = HalReadPerCPU(kiProcessorContext.irq_counter);
	return (cnt & KI_CONTEXT_COUNTER_EVENT_NOT_PENDING) == 0;
}

/**
 * KeSetPendingIRQ - raise a pending IRQ event on the current CPU.
 */
static inline void KeSetPendingIRQ(void)
{
	unsigned int cnt = HalReadPerCPU(kiProcessorContext.irq_counter);
	cnt &= ~KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	HalWritePerCPU(kiProcessorContext.irq_counter, cnt);
}

static inline void KeClearPendingIRQ(void)
{
	unsigned int cnt = HalReadPerCPU(kiProcessorContext.irq_counter);
	cnt |= KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	HalWritePerCPU(kiProcessorContext.irq_counter, cnt);
}

/**
 * KeEnterNoIOContext - enter the "No IO" context.
 */
static inline void KeEnterNoIOContext(void)
{
	HalIncrementPerCPU(kiProcessorContext.noio_counter);
}

/**
 * KeLeaveNoIOContext - leave the "No IO" context.
 */
static inline void KeLeaveNoIOContext(void)
{
	HalDecrementPerCPU(kiProcessorContext.noio_counter);
}

/**
 * KeInNoIOContext - check if the current CPU is in a "No IO" context.
 * Returns true if the current CPU is in the "No IO" context.
 */
static inline bool KeInNoIOContext(void)
{
	return HalReadPerCPU(kiProcessorContext.noio_counter) != 0;
}

void KeEnterIRQContextFromUserspace(void);

void KeExitIRQContextToUserspace(void);

bool KeEnterIRQContextFromKernel(unsigned int vector);

void KeExitIRQContextToKernel(void);

