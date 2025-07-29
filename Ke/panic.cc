// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/panic.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/interrupt.h>
#include <Ke/context.h>
#include <Ke/log.h>
#include <Ki/log.h>
#include <davix/atomic.h>
#include <davix/export.h>
#include <davix/vsnprintf.h>

static void panic_stop_self [[noreturn]] (void)
{
	for (;;) {
		HalDisableRawIRQs();
		HalWaitForInterrupt();
	}
}

static void acquire_panic_context(void)
{
	HalDisableRawIRQs();
	KeDisablePreemption();
	KeDisableDPCs();
	KeDisableIRQs();
}

static char panic_msg_buffer[1024];

void KePanic(const char *fmt, ...)
{
	acquire_panic_context();

	va_list args;
	va_start(args, fmt);
	vsnprintf(panic_msg_buffer, sizeof(panic_msg_buffer), fmt, args);
	va_end(args);
	panic_msg_buffer[sizeof(panic_msg_buffer) - 1] = 0;

	KiBeginPanicLogging();
	KePuts("KERNEL PANIC!\n");
	KePuts("What: ");
	KePuts(panic_msg_buffer);
	KePuts("\nHalting...\n");
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

