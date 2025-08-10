// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/init.cc
 * Kernel initialization.
 */
#include <Acpi/setup.h>
#include <Hal/percpu.h>
#include <Ke/context.h>
#include <Ke/idle.h>
#include <Ke/log.h>
#include <Ke/timer.h>
#include <Ki/start_kernel.h>
#include <Mm/pool.h>

#define stringize(macro) stringize_(macro)
#define stringize_(macro) #macro

static const char davix_banner[] = "rtdavix"
	" (" stringize(COMPILE_USER) "@" stringize(COMPILE_HOST) ")"
	" (" stringize(CC_VERSION) ")\n";

#undef stringize
#undef stringize_

/**
 * KiInitializeEarlySubsystems - perform very early subsystem initialization.
 *
 * This runs before KiStartKernel and shall only perform static initialization.
 */
void KiInitializeEarlySubsystems(void)
{
	KiProcessorContext *ctx = HalPtrThisCpu(kiProcessorContext);
	ctx->noio_counter = 0;
	ctx->preemption_counter = KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	ctx->dpc_counter = KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;
	ctx->irq_counter = KI_CONTEXT_COUNTER_EVENT_NOT_PENDING;

	HalInitializePerCPUVariables(0);
}

static void timer_a_func(KTIMER *timer)
{
	KePrintf("KTIMER1 hello!\n");
	KeSetTimer(timer, HalReadSchedClock() + 1000000000ULL);
}

static void timer_b_func(KTIMER *timer)
{
	KePrintf("KTIMER2 hello!\n");
	KeSetTimer(timer, HalReadSchedClock() + 5000000000);
}

static KTIMER timer_a;
static KTIMER timer_b;

/**
 * KiStartKernel - start the kernel.
 */
void KiStartKernel(void)
{
	KePuts(davix_banner);

	MmInitializeObjectAllocator();

	AcpiInitializeTables();

	HalInitialize();

	timer_a.init(timer_a_func);
	timer_b.init(timer_b_func);

	KeSetTimer(&timer_a, HalReadSchedClock() + 1000000000ULL);
	KeSetTimer(&timer_b, HalReadSchedClock() + 1000000000ULL);

	KeCPUIdleLoop();
}

