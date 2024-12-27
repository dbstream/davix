/**
 * SMP support
 * Copyright (C) 2024  dbstream
 */
#include <davix/atomic.h>
#include <davix/context.h>
#include <davix/cpuset.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/smp.h>
#include <davix/spinlock.h>
#include <davix/stdbool.h>
#include <davix/workqueue.h>
#include <asm/ipi.h>
#include <asm/irq.h>
#include <asm/sections.h>
#include <asm/smp.h>
#include <asm/smpboot.h>

unsigned int nr_cpus = 1;

cpuset_t cpus_online;
cpuset_t cpus_present;

static bool disable_smpboot = false;

__INIT_TEXT
void
smp_init (void)
{
	printk (PR_INFO "CPUs: %u\n", nr_cpus);
}

__INIT_TEXT
void
smpboot_init (void)
{
	if (nr_cpus == 1)
		return;

	errno_t e = arch_smpboot_init ();
	if (e != ESUCCESS) {
		printk (PR_ERR "smp: arch_smpboot_init failed with error %d\n", e);
		printk (PR_WARN "smp: will not attempt SMPBOOT\n");
		disable_smpboot = true;
	}
}

__INIT_TEXT
void
smp_boot_cpus (void)
{
	if (disable_smpboot)
		return;

	for_each_present_cpu (cpu) {
		if (cpu_online (cpu))
			continue;

		errno_t e = arch_smp_boot_cpu (cpu);
		if (e != ESUCCESS)
			printk (PR_ERR "smp: smp_boot_cpu(%u) failed with error %d\n", cpu, e);
	}
}

void
smp_start_additional_cpu (void)
{
	preempt_off ();
	irq_enable ();

	sched_init_this_cpu ();
	workqueue_init_this_cpu ();

	sched_idle ();
}

struct call_on_cpu_state {
	spinlock_t lock;
	struct smp_call_on_cpu_work *head;
	struct smp_call_on_cpu_work **tail;
};

static __CPULOCAL struct call_on_cpu_state call_on_cpu_state;

void
smp_do_call_on_cpu_work (void)
{
	struct call_on_cpu_state *state = this_cpu_ptr (&call_on_cpu_state);
	__spin_lock (&state->lock);

	struct smp_call_on_cpu_work *next, *work = state->head;
	state->head = NULL;
	state->tail = NULL;

	__spin_unlock (&state->lock);

	while (work) {
		next = work->next;
		work->func (work->arg);
		atomic_store_release (&work->done, 1);
		work = next;
	}
}

void
smp_call_on_cpu_async (unsigned int cpu, struct smp_call_on_cpu_work *work)
{
	struct call_on_cpu_state *state = that_cpu_ptr (&call_on_cpu_state, cpu);
	work->next = NULL;
	work->done = 0;

	spin_lock (&state->lock);
	if (state->tail) {
		*state->tail = work;
		state->tail = &work->next;
	} else {
		state->head = work;
		state->tail = &work->next;
	}
	spin_unlock (&state->lock);

	if (cpu == this_cpu_id ()) {
		bool flag = irq_save ();
		smp_do_call_on_cpu_work ();
		irq_restore (flag);
	} else
		arch_send_call_on_cpu_IPI (cpu);
}

void
smp_wait_for_call_on_cpu (struct smp_call_on_cpu_work *work)
{
	do {
		arch_relax ();
	} while (!atomic_load_acquire (&work->done));
}
