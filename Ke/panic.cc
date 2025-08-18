// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/panic.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/interrupt.h>
#include <Hal/ipi.h>
#include <Hal/smp.h>
#include <Ke/context.h>
#include <Ke/log.h>
#include <Ke/smp.h>
#include <Ki/log.h>
#include <davix/atomic.h>
#include <davix/export.h>
#include <davix/vsnprintf.h>
#include <string.h>

static DEFINE_PERCPU(bool, stopped);

static void panic_stop_self [[noreturn]] (void)
{
	HalWritePerCPU(stopped, true);
	for (;;) {
		HalDisableRawIRQs();
		HalWaitForInterrupt();
	}
}

static unsigned int panicking_cpu = -1U;

static void acquire_panic_context(void)
{
	HalDisableRawIRQs();
	KeDisablePreemption();
	KeDisableDPCs();
	KeDisableIRQs();

	unsigned int self = HalCurrentProcessor();

	unsigned int expected = -1U;
	bool status = atomic_cmpxchg(
		&panicking_cpu,
		&expected,
		self,
		_MO_SeqCst,
		_MO_SeqCst
	);

	if (!status) {
		if (expected == self) {
			// nested panic... something has gone really wrong.
			// TODO: dump something on 0xe9 if running virtualized.
		}

		panic_stop_self();
	}

	/*
	 * Signal any online CPUs to stop running.
	 */
	for (unsigned int cpu = 0; cpu < keProcessorCount; cpu++) {
		if (cpu == self)
			continue;
		/*
		 * NB: usually the hotplug lock must be held to access the CPU
		 * online bitmaps, but in a panic condition taking locks is
		 * maybe not the best idea...
		 */
		if (!KeCPUOnline(cpu))
			continue;

		HalSendPanicIPI(cpu);
	}

	/*
	 * Wait for them to enter panic_stop_self.
	 */
	for (unsigned int cpu = 0; cpu < keProcessorCount; cpu++) {
		if (cpu == self)
			continue;

		bool *p = HalPtrPerCPU(stopped, cpu);

		/*
		 * NB: same as above, we really should take the hotplug lock
		 * here but we don't.
		 */
		while (KeCPUOnline(cpu)) {
			if (atomic_load(p, _MO_SeqCst))
				break;

			smp_spinwait_hint();
		}
	}
}

static char panic_msg_buffer[1024];

void KePanic(const char *fmt, ...)
{
	acquire_panic_context();

	va_list args;
	va_start(args, fmt);
	vsnprintf(panic_msg_buffer, sizeof(panic_msg_buffer) - 1, fmt, args);
	va_end(args);
	size_t n = strlen(panic_msg_buffer);
	panic_msg_buffer[n] = '\n';
	panic_msg_buffer[n + 1] = '\0';

	KiBeginPanicLogging();
	KePuts("KERNEL PANIC!\n");
	KePuts("What:\n");
	KePuts(panic_msg_buffer);
	KePuts("Halting...\n");
	KiEndPanicLogging();

	panic_stop_self();
}
EXPORT_SYMBOL(KePanic)

extern "C"
void __stack_chk_fail(void)
{
	KePanic("*** stack smashing detected ***");
}
EXPORT_SYMBOL(__stack_chk_fail)

