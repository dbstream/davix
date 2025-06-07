/**
 * Simultaneous Multiprocessing (SMP) support.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/percpu.h>
#include <asm/smp.h>
#include <davix/atomic.h>
#include <davix/cpuset.h>
#include <davix/irql.h>
#include <davix/printk.h>
#include <davix/smp.h>
#include <davix/spinlock.h>
#include <dsl/list.h>

void
smp_boot_all_cpus (void)
{
	if (nr_cpus == 1)
		return;

	printk (PR_NOTICE "SMP: Bringing %u additional CPU(s) online...\n", nr_cpus - 1);

	unsigned int nr_onlined = 0;
	for (unsigned int cpu : cpu_present) {
		if (cpu_online (cpu))
			continue;

		if (!arch_smp_boot_cpu (cpu)) {
			printk (PR_ERROR "SMP: Failed to bring CPU%u online; cancelling SMPBOOT\n", cpu);
			break;
		}

		nr_onlined++;
	}

	printk (PR_NOTICE "SMP: Brought %u additional CPU(s) online\n", nr_onlined);
}

struct call_on_cpu_data {
	dsl::ListHead list;
	void (*fn)(void *);
	void *arg;
	bool completion;
};

typedef dsl::TypedList<call_on_cpu_data, &call_on_cpu_data::list> SMPCallList;

struct call_on_cpu_control_block {
	SMPCallList callback_list;
	spinlock_t lock;
};

static DEFINE_PERCPU(call_on_cpu_control_block, smp_call_block);

PERCPU_CONSTRUCTOR(smpcall)
{
	call_on_cpu_control_block *cb = percpu_ptr (smp_call_block).on (cpu);

	cb->callback_list.init ();
	cb->lock.init ();
}

static void
wait_for_call_on_cpu (call_on_cpu_data *data)
{
	while (!atomic_load_acquire (&data->completion)) {
		smp_spinlock_hint ();
	}
}

static void
complete_call_on_cpu (call_on_cpu_data *data)
{
	atomic_store_release (&data->completion, true);
}

/**
 * smp_handle_call_on_one_ipi - dispatch SMP calls on the current CPU.
 */
void
smp_handle_call_on_one_ipi (void)
{
	call_on_cpu_control_block *cb = percpu_ptr (smp_call_block);
	SMPCallList call_list;

	cb->lock.raw_lock ();
	call_list.adopt (&cb->callback_list);
	cb->lock.raw_unlock ();

	while (!call_list.empty ()) {
		call_on_cpu_data *data = call_list.pop_front ();
		data->fn (data->arg);
		complete_call_on_cpu (data);
	}
}

/**
 * smp_call_on_cpu - call a function on another processor in an SMP system.
 * @cpu: cpu number of processor to call on
 * @fn: function to call; this must be a fast IRQ-safe function
 * @arg: argument to pass to the function
 *
 * This function must be called at DPC level or below.
 */
void
smp_call_on_cpu (unsigned int cpu, void (*fn)(void *), void *arg)
{
	disable_dpc ();
	if (cpu == this_cpu_id ()) {
		fn (arg);
		enable_dpc ();
		return;
	}

	call_on_cpu_data data;
	data.fn = fn;
	data.arg = arg;
	data.completion = false;

	call_on_cpu_control_block *cb = percpu_ptr (smp_call_block).on (cpu);

	cb->lock.lock_irq_atomic ();
	bool was_empty = cb->callback_list.empty ();
	cb->callback_list.push_back (&data);
	cb->lock.unlock_irq ();

	if (was_empty)
		arch_send_smp_call_on_one_IPI (cpu);

	enable_dpc ();
	wait_for_call_on_cpu (&data);
}
