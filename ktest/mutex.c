/**
 * Stress-test the mutex implementation by torturing it.
 * Copyright (C) 2024  dbstream
 *
 * Why do we have to be so mean to our code?  :^(   /David
 */

#if CONFIG_KTEST_MUTEX

#include <davix/atomic.h>
#include <davix/cpuset.h>
#include <davix/mutex.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/snprintf.h>
#include <davix/smp.h>
#include <davix/task_api.h>
#include <asm/smp.h>

#define N 3

static unsigned int num_active_workers;
static unsigned int started_workers;

struct mutextorture_data {
	struct task *task;
};

static __CPULOCAL struct mutextorture_data mutextorture_data[N];

static struct mutex mutex;
static int guarded_variable;

	// poor man's random number generator
static unsigned int
rng (unsigned int *state)
{
	unsigned int s = *state;

	s = (s << 5) ^ (s >> 12) ^ (s >> 13);
	if (!(s & 1))
		s ^= 0x8ecc013fU;

	*state = s;
	return s;
}

// #define mutex_lock(x)
// #define mutex_unlock(x)

static void
mutextorture (void *arg)
{
	sched_begin_task ();

	struct mutextorture_data *data = arg;

	unsigned int rng_state = 0xdeadbeefU + (unsigned int) ns_since_boot ();

	atomic_inc_fetch_relaxed (&started_workers);
	do {
		arch_relax ();
		schedule ();
	} while (atomic_load_relaxed (&started_workers) < atomic_load_relaxed (&num_active_workers));

	for (int i = 0; i < 10000; i++) {
		int j = rng (&rng_state) & 1023;
		if (j & 1)
			schedule ();
		else
			for (; j; j--)
				arch_relax ();

		mutex_lock (&mutex);
		if (atomic_fetch_inc_relaxed (&guarded_variable) != 0)
			panic ("mutextorture: mutual exclusion failed!");
		schedule ();

		atomic_fetch_dec_relaxed (&guarded_variable);
		mutex_unlock (&mutex);
		schedule ();
	}

	irq_disable ();
	atomic_fetch_dec_relaxed (&num_active_workers);
	data->task->state = TASK_ZOMBIE;
	schedule ();
}

static void
enqueue_mutextorture_task (void *arg)
{
	(void) arg;

	for (unsigned int i = 0; i < N; i++) {
		struct mutextorture_data *data = this_cpu_ptr (&mutextorture_data[i]);

		char comm[16];
		snprintf (comm, sizeof (comm), "torture-%u-%u", this_cpu_id (), i);
		data->task = create_kernel_task (comm, mutextorture, data, TF_NOMIGRATE);
		if (!data->task)
			panic ("mutextorture: failed to create task %s", comm);
	}
}

void
ktest_mutex (void)
{
	mutex_init (&mutex);

	preempt_off ();
	num_active_workers = N * nr_cpus;
	printk (PR_INFO "Running mutextorture with %u workers...\n", num_active_workers);
	for_each_online_cpu (cpu)
		smp_call_on_cpu (cpu, enqueue_mutextorture_task, NULL);
	preempt_on ();

	printk ("starting wait...\n");
	do {
		schedule ();
		arch_relax ();
	} while (atomic_load_relaxed (&num_active_workers) != 0);
	printk (PR_INFO "Finished mutextorture.\n");
}

#endif
