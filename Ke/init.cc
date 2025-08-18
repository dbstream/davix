// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/init.cc
 * Kernel initialization.
 */
#include <Acpi/setup.h>
#include <Ex/thread.h>
#include <Hal/percpu.h>
#include <Hal/smpboot.h>
#include <Ke/context.h>
#include <Ke/idle.h>
#include <Ke/log.h>
#include <Ke/sched.h>
#include <Ke/thread.h>
#include <Ke/timer.h>
#include <Ki/sched.h>
#include <Ki/start_kernel.h>
#include <Mm/pool.h>
#include <davix/bug.h>
#include <davix/vsnprintf.h>

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

static unsigned int taskHeartbeatDistr[10];

static void timer_b_func(KTIMER *timer)
{
	KePrintf("KTIMER2 hello!\n");
	KePrintf("%4u %4u %4u %4u %4u %4u %4u %4u %4u %4u\n",
		taskHeartbeatDistr[0],
		taskHeartbeatDistr[1],
		taskHeartbeatDistr[2],
		taskHeartbeatDistr[3],
		taskHeartbeatDistr[4],
		taskHeartbeatDistr[5],
		taskHeartbeatDistr[6],
		taskHeartbeatDistr[7],
		taskHeartbeatDistr[8],
		taskHeartbeatDistr[9]
	);
	KeSetTimer(timer, HalReadSchedClock() + 5000000000);
}

static KTIMER timer_a;
static KTIMER timer_b;

static void start_init_thread(void *arg);

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

	KiInitializeScheduler();
	HalSmpStartProcessors();

	ETHREAD *init_thread;
	OSSTATUS status = ExCreateThread(
		&init_thread,
		start_init_thread,
		nullptr
	);

	if (!OS_SUCCESS(status))
		KePanic("Failed to create init thread: %d\n", status);

	BUG_ON(!ExWakeThread(init_thread));

	KeCPUIdleLoop();
}

static void loop_forever(void *arg)
{
	int prio = (int) (unsigned long) arg;

	KeSetBasePriority(prio);

	for (;;) {
		for (int i = 0; i < 1000000000; i++)
			asm volatile("" ::: "memory");
		taskHeartbeatDistr[prio]++;
		KePrintf("%d\n", prio);

		if (taskHeartbeatDistr[prio] > 20)
			KePanic("panic test");
	}
}

static void start_init_thread(void *arg)
{
	(void) arg;

	KePrintf("Hello from init thread!\n");

	KeDisablePreemption();

	for (int i = 0; i < 10; i++) {
		char comm[32];
		snprintf(comm, sizeof(comm), "test:%03d", i);
		ETHREAD *thread;
		OSSTATUS status = ExCreateThread(&thread, loop_forever, (void *) (unsigned long) i);
		if (!OS_SUCCESS(status))
			KePanic("Failed to create thread %s: %d\n", comm, status);
		ExSetThreadComm(thread, comm);
		BUG_ON(!ExWakeThread(thread));
	}

	KeSetCurrentState(KTHREAD_ZOMBIE);
	KeReschedule();
	KePanic("KeReschedule() returned!");
}

