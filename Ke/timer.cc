// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/timer.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/interrupt.h>
#include <Hal/percpu.h>
#include <Ke/context.h>
#include <Ke/dpc.h>
#include <Ke/irq.h>
#include <Ke/timer.h>
#include <dsl/avltree.h>

struct ktimer_compare {
	constexpr bool operator()(const KTIMER *lhs, const KTIMER *rhs) const
	{
		return lhs->expiry < rhs->expiry;
	}
};

typedef dsl::TypedAVLTree<KTIMER, &KTIMER::node, ktimer_compare> KTimerTree;

struct KTimerData {
	nsec_t expiry;
	KTimerTree tree;
	DPC dpc;
};

static DEFINE_PERCPU(KTimerData, kiTimerData);

static void timer_dpc_function(DPC *dpc, void *context, void *arg1, void *arg2)
{
	(void) dpc;
	(void) context;
	(void) arg2;

	KTimerData *data = (KTimerData *) arg1;
	nsec_t now = HalReadSchedClock();

	for (;;) {
		KTIMER *timer = data->tree.first();
		if (!timer)
			return;

		nsec_t expiry = timer->expiry;
		if (now < expiry)
			now = HalReadSchedClock();
		if (now < expiry) {
			KeDisableIRQs();
			data->expiry = expiry;
			KeEnableIRQs();
			return;
		}

		data->tree.remove(timer);
		timer->on_queue = false;
		timer->callback(timer);
	}
}

HAL_PERCPU_CALLBACK(cpu)
{
	KTimerData *data = HalPtrPerCPU(kiTimerData, cpu);

	data->expiry = NSEC_MAX;
	data->tree.init();
	data->dpc.init(timer_dpc_function, data, nullptr);
}

/**
 * KeSetTimer - register a KTIMER.
 * @timer: KTIMER object
 * @expiry: nanoseconds since boot when the timer expires
 * Returns false if the timer is already registered.
 *
 * If the KTIMER is already registered, this function has to be called on the
 * same CPU that registered it first.
 */
bool KeSetTimer(KTIMER *timer, nsec_t expiry)
{
	KeDisableDPCs();
	if (timer->on_queue) {
		KeEnableDPCs();
		return false;
	}

	timer->on_queue = true;
	timer->expiry = expiry;

	KTimerData *data = HalPtrThisCpu(kiTimerData);

	KeDisableIRQs();
	if (expiry < data->expiry)
		data->expiry = expiry;
	KeEnableIRQs();

	data->tree.insert(timer);

	KeEnableDPCs();
	return true;
}

/**
 * KeUnsetTimer - deregister a KTIMER.
 * @timer: KTIMER object
 * Returns false if the timer was not registered.
 *
 * If the KTIMER is registered, this function has to be called on the same CPU
 * that registered it.
 */
bool KeUnsetTimer(KTIMER *timer)
{
	KeDisableDPCs();
	if (!timer->on_queue) {
		KeEnableDPCs();
		return false;
	}

	timer->on_queue = false;

	KTimerData *data = HalPtrThisCpu(kiTimerData);

	data->tree.remove(timer);

	KeEnableDPCs();
	return true;
}

/**
 * KeHandleLocalTimerInterrupt - handle the Local APIC Timer interrupt.
 */
void KeHandleLocalTimerInterrupt(void)
{
	nsec_t expiry = HalReadPerCPU(kiTimerData.expiry);
	nsec_t now = HalReadSchedClock();

	if (now >= expiry) {
		HalWritePerCPU(kiTimerData.expiry, NSEC_MAX);
		DPC *dpc = HalPtrThisCpu(kiTimerData.dpc);
		KeEnqueueDPC(dpc, nullptr);
	}
}

