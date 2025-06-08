/**
 * panic() - the kernel's "bug check".
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/asm.h>
#include <asm/irql.h>
#include <asm/smp.h>
#include <davix/atomic.h>
#include <davix/cpuset.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/time.h>
#include <vsnprintf.h>

static unsigned int panicking_cpu = -1U;

bool
in_panic (void)
{
	return atomic_load_relaxed (&panicking_cpu) != -1U;
}

static void
panic_stop_self (void)
{
	raw_irq_disable ();

	smp_mb ();
	cpu_online.clear (this_cpu_id ());

	for (;;)
		wait_for_interrupt ();
}

/**
 * enter_panic - acquire the panic context.
 * Returns true if other CPUs were slow to self stop, otherwise false.  Doesn't
 * return if someone else got the panic context instead.
 */
static bool
enter_panic (void)
{
	raw_irq_disable ();

	unsigned int me = this_cpu_id ();

	unsigned int expected = -1U;
	unsigned int desired = me;
	bool ret = atomic_cmpxchg (&panicking_cpu,
			&expected, desired, mo_seq_cst, mo_seq_cst);

	if (!ret)
		panic_stop_self ();

	arch_send_panic_IPI_to_others ();

	nsecs_t deadline = ns_since_boot () + 1000000000ULL; // 1 second

	for (unsigned int cpu : cpu_online) {
		if (cpu == me)
			continue;

		do {
			if (ns_since_boot () > deadline)
				goto slow_cpus;
			smp_spinlock_hint ();
		} while (cpu_online (cpu));
	}
	smp_mb ();
	return false;

slow_cpus:
	arch_send_panic_NMI_to_others ();

	for (unsigned int cpu : cpu_online) {
		if (cpu == me)
			continue;

		do {
			smp_spinlock_hint ();
		} while (cpu_online (cpu));
	}
	smp_mb ();
	return true;
}

static char panic_buf[768];

extern "C"
void
panic (const char *fmt, ...)
{
	bool warn_slow = enter_panic ();

	va_list args;

	va_start (args, fmt);
	vsnprintf (panic_buf, sizeof (panic_buf), fmt, args);
	va_end (args);

	printk (PR_ERROR "--- kernel PANIC ---\n");
	if (warn_slow)
		printk (PR_WARN "warning: other CPUs took >1s to enter idle loop\n");
	printk (PR_ERROR "what: %.768s\n", panic_buf);
	printk (PR_ERROR "--- end kernel PANIC ---\n");

	panic_stop_self ();
}

extern "C"
void
__stack_chk_fail (void)
{
	panic ("*** stack smashing detected ***");
}
