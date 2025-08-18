// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/context.cc
 * KiProcessorContext and dispatch routines.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/interrupt.h>
#include <Hal/percpu.h>
#include <Ke/context.h>
#include <Ke/dpc.h>
#include <Ke/irq.h>
#include <Ke/sched.h>
#include <davix/atomic.h>
#include <davix/bug.h>
#include <davix/export.h>

typedef dsl::TypedList<DPC, &DPC::list_entry> DPC_List;

DEFINE_PERCPU(KiProcessorContext, kiProcessorContext);

static DEFINE_PERCPU(DPC_List, kiLocalDPCList);

HAL_PERCPU_CALLBACK(cpu)
{
	DPC_List *dpc_list = HalPtrPerCPU(kiLocalDPCList, cpu);
	dpc_list->init();

	/*
	 * Skip setting up the BSP's KiProcessorContext.  That is done for us
	 * already by KiInitializeEarlySubsystems.
	 */
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
	BUG_ON(!KeDPCsEnabled());
	KeReschedule();
}
EXPORT_SYMBOL(KeDispatchPendingPreemption)

/**
 * KeDispatchPendingDPCs - dispatch pending deferred procedure calls (DPCs).
 */
void KeDispatchPendingDPCs(void)
{
	DPC_List tmp;
	DPC_CALLBACK callback;
	DPC *dpc;
	void *context;
	void *arg1;
	void *arg2;

	/*
	 * Disable preemption for the entire duration of this function.  It is
	 * implicitly masked when entering this function anyways, but doesn't
	 * hurt to do explicitly.  It is also a good idea to do this so that a
	 * DPC which sets the pending preemption flag doesn't reschedule
	 * immediately, potentially increasing latency for other enqueued DPCs.
	 */
	KeDisablePreemption();
	/*
	 * Preemption is disabled -- the CPU cannot change.
	 */
	DPC_List *dpc_list = HalPtrThisCpu(kiLocalDPCList);
	do {
		KeDisableDPCs();
		/*
		 * Disable IRQs when copying out from the DPC list.
		 */
		KeDisableIRQs();
		KeClearPendingDPC();
		tmp.adopt(dpc_list);
		KeEnableIRQs();

		while ((dpc = tmp.pop_front())) {
			callback = dpc->callback;
			context = dpc->context;
			arg1 = dpc->arg1;
			arg2 = dpc->arg2;
			/*
			 * Compiler optimization barrier: finish removing the
			 * DPC from the queue and reading callback, context,
			 * arg1 and arg2 before storing false to dpc->on_queue.
			 */
			barrier();
			atomic_store_relaxed(&dpc->on_queue, false);
			/*
			 * Invoke the callback now.
			 */
			callback(dpc, context, arg1, arg2);
		}
	} while (KiEnableDPCsAndTest());

	/*
	 * Reenable preemption before exiting.
	 */
	KeEnablePreemption();
}
EXPORT_SYMBOL(KeDispatchPendingDPCs)

/**
 * KeEnqueueDPC - enqueue a deferred procedure call (DPC).
 * @dpc: pointer to a DPC structure
 * @context: context argument
 * Returns false if the DPC was already enqueued.
 *
 * If this function returns true, the callback function will be invoked with
 * @context as the context argument.  A DPC which has been enqueued multiple
 * times, with the latter KeEnqueueDPC calls returning false and effectively
 * being no-ops, will use the context argument from the first KeEnqueueDPC call
 * that succeeded.
 */
bool KeEnqueueDPC(DPC *dpc, void *context)
{
	KeDisableIRQs();
	if (dpc->on_queue) {
		KeEnableIRQs();
		return false;
	}

	dpc->on_queue = true;
	dpc->context = context;
	HalPtrThisCpu(kiLocalDPCList)->push_back(dpc);
	KeSetPendingDPC();
	KeEnableIRQs();
	return true;
}
EXPORT_SYMBOL(KeEnqueueDPC)

static DEFINE_PERCPU(unsigned int, kiPendingVector);

void KeDispatchPendingIRQs(void)
{
	KeDisablePreemption();
	KeDisableDPCs();
	do {
		KeDisableIRQs();
		KeClearPendingIRQ();
		KeHandleInterruptVector(HalReadPerCPU(kiPendingVector));
	} while (KiEnableIRQsAndTest());
	HalEnableRawIRQs();
	KeEnableDPCs();
	KeEnablePreemption();
}
EXPORT_SYMBOL(KeDispatchPendingIRQs)

/**
 * KeEnterIRQContextFromUserspace - enter IRQ handler context from user mode.
 *
 * This routine is called by the architecture's low-level interrupt vector entry
 * point(s) before calling KeHandleInterruptVector if the interrupt struck while
 * we were executing in user mode.
 */
void KeEnterIRQContextFromUserspace(void)
{
	KeDisablePreemption();
	KeDisableDPCs();
}

/**
 * KeExitIRQContextToUserspace - leave IRQ handler context to user mode.
 *
 * This routine is called by the architecture's low-level interrupt vector entry
 * point(s) after KeHandleInterruptVector returns if the interrupt struck while
 * we were executing in user mode.
 */
void KeExitIRQContextToUserspace(void)
{
	/*
	 * Interrupts need to be enabled when dispatching DPCs or when entering
	 * the scheduler.  But we must disable them before leaving kernel-mode.
	 */

	if (KiEnableDPCsAndTest()) {
		HalEnableRawIRQs();
		KeDispatchPendingDPCs();
		KeEnablePreemption();
		HalDisableRawIRQs();
	} else if (KiEnablePreemptionAndTest()) {
		HalEnableRawIRQs();
		KeDispatchPendingPreemption();
		HalDisableRawIRQs();
	}
}

static DEFINE_PERCPU(unsigned int, kiIRQEntryFromKernelFlags);

/**
 * KeEnterIRQContextFromKernel - enter IRQ context from kernel mode.
 * @vector: interrupt vector number
 * Returns false if IRQs were lazy-disabled and interrupt handling is deferred.
 *
 * This routine is called by the architecture's low-level interrupt vector entry
 * point(s) before calling KeHandleInterruptVector if the interrupt struck while
 * we were executing in kernel mode.
 */
bool KeEnterIRQContextFromKernel(unsigned int vector)
{
	if (!KeIRQsEnabled()) {
		/*
		 * IRQs are lazy disabled; store the pending interrupt and exit
		 * with interrupts disabled.
		 */
		KeSetPendingIRQ();
		HalWritePerCPU(kiPendingVector, vector);
		return false;
	}

	unsigned int preempt_cnt = HalReadPerCPU(kiProcessorContext.preemption_counter);
	unsigned int dpc_cnt = HalReadPerCPU(kiProcessorContext.dpc_counter);

	KeDisablePreemption();
	KeDisableDPCs();

	unsigned int flags = 0U;
	if (preempt_cnt != 0)
		flags |= 1U;
	if (dpc_cnt != 0)
		flags |= 2U;

	HalWritePerCPU(kiIRQEntryFromKernelFlags, flags);
	return true;
}

/**
 * KeExitIRQContextToKernel - leave IRQ handler context to kernel mode.
 *
 * This routine is called by the architecture's low-level interrupt vector entry
 * point(s) after KeHandleInterruptVector returns if the interrupt struck while
 * we were executing in kernel mode.  If KeEnterIRQContextFromKernel returns
 * false, this function is not called.
 */
void KeExitIRQContextToKernel(void)
{
	/*
	 * Interrupts need to be enabled when dispatching DPCs or when entering
	 * the scheduler.  But we must disable them before returning from IRQ
	 * context.
	 *
	 * If the preempt counter or the DPC counter were already zero, as
	 * indicated by corresponding bits in kiIRQEntryFromKernelFlags, they
	 * will be dispatched by the calling code and we should do nothing.
	 */

	unsigned int flags = HalReadPerCPU(kiIRQEntryFromKernelFlags);

	if (KiEnableDPCsAndTest() && (flags & 2U)) {
		HalEnableRawIRQs();
		KeDispatchPendingDPCs();
		if (KiEnablePreemptionAndTest() && (flags & 1U))
			KeDispatchPendingPreemption();
		HalDisableRawIRQs();
	} else if (KiEnablePreemptionAndTest() && (flags & 1U)) {
		HalEnableRawIRQs();
		KeDispatchPendingPreemption();
		HalDisableRawIRQs();
	}
}

