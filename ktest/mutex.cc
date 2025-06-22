/**
 * ktests for mutex acquisition and release.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/irql.h>
#include <davix/atomic.h>
#include <davix/kthread.h>
#include <davix/mutex.h>
#include <davix/printk.h>
#include <davix/time.h>

static Mutex mutex;
static int guarded_variable;

static int num_active_workers;

static int maximum_concurrent;

static void
mutextorture (void *arg)
{
	(void) arg;

	for (int i = 0; i < 100000; i++) {
		mutex.lock ();
		int n = atomic_inc_fetch (&guarded_variable, mo_relaxed);

		int expected = 0;
		while (!atomic_cmpxchg_weak (&maximum_concurrent, &expected, n,
				mo_relaxed, mo_relaxed)) {
			if (n <= expected)
					break;
		}

		for (int i = 0; i < 10; i++)
			smp_spinlock_hint ();

		atomic_dec_fetch (&guarded_variable, mo_relaxed);
		mutex.unlock ();

		for (int i = 0; i < 10; i++)
			smp_spinlock_hint ();
	}

	atomic_dec_fetch (&num_active_workers, mo_release);
	kthread_exit ();
}

void
ktest_mutex (void)
{
	printk (PR_NOTICE "Running mutex ktest...\n");

	disable_dpc ();
	for (int i = 0; i < 100; i++) {
		Task *task = kthread_create ("mutextorture", mutextorture,
				nullptr);

		if (!task) {
			printk (PR_ERROR "ktest_mutex: failed to create a kthread\n");
			break;
		}

		atomic_inc_fetch (&num_active_workers, mo_relaxed);
		kthread_start (task);
	}
	enable_dpc ();

	while (atomic_load_acquire (&num_active_workers) != 0)
		smp_spinlock_hint ();

	int n = maximum_concurrent;
	if (n == 1)
		printk (PR_INFO "Mutex ktest succeeded.\n");
	else
		printk (PR_ERROR "Mutex ktest failed: maximum_concurrent=%d\n", n);
}

