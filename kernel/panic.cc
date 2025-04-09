/**
 * panic() - the kernel's "bug check".
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/panic.h>
#include <davix/printk.h>
#include <vsnprintf.h>

static void
enter_panic (void)
{
	asm volatile ("cli" ::: "memory");
}

static void
panic_stop_self (void)
{
	asm volatile ("cli" ::: "memory");
	for (;;)
		asm volatile ("hlt" ::: "memory");
}

extern "C"
void
panic (const char *fmt, ...)
{
	char buf[128];
	va_list args;

	va_start (args, fmt);
	vsnprintf (buf, sizeof (buf), fmt, args);
	va_end (args);

	enter_panic ();

	printk (PR_ERROR "--- kernel PANIC ---\n");
	printk (PR_ERROR "what: %.128s\n", buf);
	printk (PR_ERROR "--- end kernel PANIC ---\n");

	panic_stop_self ();
}

extern "C"
void
__stack_chk_fail (void)
{
	panic ("*** stack smashing detected ***");
}
