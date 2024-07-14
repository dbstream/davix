/**
 * Dealing with kernel panics.
 * Copyright (C) 2024  dbstream
 */
#include <davix/atomic.h>
#include <davix/context.h>
#include <davix/cpuset.h>
#include <davix/spinlock.h>
#include <davix/stdarg.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/snprintf.h>
#include <asm/irq.h>
#include <asm/smp.h>

/**
 * panic callbacks
 *
 * Due to the super-atomicity of kernel panics, the list head and cb->next
 * fields must always be in a valid state, as these are used to iterate the
 * callback list during panic().
 *
 * The spinlock defined below is not used by panic_invoke_callbacks.
 */

static struct panic_callback *panic_callbacks;
static spinlock_t panic_callbacks_lock;

static void
panic_invoke_callbacks (void)
{
	struct panic_callback *cb = atomic_load_acquire (&panic_callbacks);
	while (cb) {
		cb->callback ();
		cb = atomic_load_acquire (&cb->next);
	}
}

void
register_panic_callback (struct panic_callback *cb)
{
	bool flag = spin_lock_irq (&panic_callbacks_lock);
	cb->next = panic_callbacks;
	cb->link = &panic_callbacks;
	if (panic_callbacks)
		panic_callbacks->link = &cb->next;

	atomic_store_release (&panic_callbacks, cb);
	spin_unlock_irq (&panic_callbacks_lock, flag);
}

void
deregister_panic_callback (struct panic_callback *cb)
{
	bool flag = spin_lock_irq (&panic_callbacks_lock);
	if (cb->next)
		cb->next->link = cb->link;

	atomic_store_release (cb->link, cb->next);
	spin_unlock_irq (&panic_callbacks_lock, flag);
}

/**
 * Put the current CPU into an idling loop.
 */
static void
panic_stop_self (void)
{
	irq_disable ();

	/* mark us as offline so enter_panic can proceed */
	clear_cpu_online (this_cpu_id ());

	for (;;)
		arch_wfi ();
}

static unsigned int panic_cpu = -1U;

int
in_panic (void)
{
	return atomic_load_relaxed (&panic_cpu) != -1U;
}

void
panic_nmi_handler (void)
{
	/**
	 * Only enter the idling loop if this is not the panic CPU. This stops
	 * bad things from happening if the panic CPU gets hit by an NMI.
	 */
	if (atomic_load_relaxed (&panic_cpu) != this_cpu_id ())
		panic_stop_self ();
}

/**
 * Enter the panic context. Only one CPU can be in the panic context at a time,
 * and when it is, all other CPUs are halted.
 */
static void
enter_panic (void)
{
	irq_disable ();
	preempt_off ();

	unsigned int expected = -1U;
	if (!atomic_cmpxchg_relaxed (&panic_cpu, &expected, this_cpu_id ()))
		/**
		 * Another CPU acquired the panic context before us.
		 */
		panic_stop_self ();

	/**
	 * We are now the panic CPU. Stop all other CPUs.
	 */

	/* TODO: use IPIs to stop all other CPUs */
}

static void
__panic (const char *msg)
{
	enter_panic ();

	panic_invoke_callbacks ();
	printk (PR_CRIT "Kernel panic on CPU%u: %s\n", this_cpu_id (), msg);
	printk (PR_CRIT "Idling...\n");
	panic_stop_self ();
}

void
panic (const char *fmt, ...)
{
	char buf[128];

	va_list args;
	va_start (args, fmt);
	vsnprintf (buf, sizeof(buf), fmt, args);
	va_end (args);

	__panic (buf);
}

void
__stack_chk_fail (void)
{
	panic ("*** stack smashing detected ***");
}
