/**
 * Segment Tree RCU
 * Copyright (C) 2025-present  dbstream
 *
 * This is Segment Tree RCU, a generational RCU implementation which has the
 * following time complexity properties:
 *
 *				average		worst-case	comment
 *	rcu_read_lock()		O(1)		O(1)		== disable_dpc()
 *	rcu_read_unlock()	O(1)		O(1)		== enable_dpc()
 *	rcu_enable()		load-dependent	O(log NCPU)
 *	rcu_disable()		load-dependent	O(log NCPU)
 *	rcu_quiesce()		O(1)		O(log NCPU)
 *
 * Generational RCU works as follows:
 *
 * - There is a global 'current generation', which is shared by every CPU.
 *
 * - RCU callbacks are enqueued on a callback list indexed by the next
 *   generation ('current_generation + 1').
 *
 * - Every CPU keeps its own 'local generation', which lags behind the global
 *   current generation.  When a CPU is quiescent, it updates the local
 *   generation to the global current generation.
 *
 * - After updating the local generation, a CPU looks at the local generation
 *   counters of all other CPUs.  If it sees that all CPUs have advanced to the
 *   global current generation, it dispatches RCU callbacks and advances the
 *   global current generation one step forwards.
 *
 * Segment Tree RCU optimizes the last step of the Generational RCU algorithm
 * described above by storing the least recent local generation count among
 * active CPUs in a segment tree, indexed by CPU number.
 *
 * This reduces the worst-case work that a CPU has to do in rcu_quiesce,
 * rcu_enable, and rcu_disable to O(log CONFIG_MAX_NR_CPUS).
 *
 * Segment Tree RCU was originally implemented without looking at any current
 * patents for RCU and RCU-like mechanisms.
 */
#include <asm/cache.h>
#include <asm/irql.h>
#include <asm/percpu.h>
#include <asm/smp.h>
#include <davix/dpc.h>
#include <davix/event.h>
#include <davix/panic.h>
#include <davix/rcu.h>
#include <davix/spinlock.h>
#include <davix/printk.h>

/**
 * rcu_read_lock - enter a RCU reader critical section.
 */
void
rcu_read_lock (void)
{
	disable_dpc ();
}

/**
 * rcu_read_unlock - exit a RCU reader critical section.
 */
void
rcu_read_unlock (void)
{
	enable_dpc ();
}

struct rcu_barrier_event {
	RCUHead rcu;
	KEvent event;
};

static void
set_rcu_barrier_event (RCUHead *rcu)
{
	rcu_barrier_event *e = container_of (&rcu_barrier_event::rcu, rcu);
	e->event.set ();
}

/**
 * rcu_barrier - wait for an RCU grace period and for callbacks to be invoked.
 */
void
rcu_barrier (void)
{
	rcu_barrier_event e;
	rcu_call (&e.rcu, set_rcu_barrier_event);
	e.event.wait ();
}

struct alignas(CACHELINE_SIZE) rcu_st_node {
	uint64_t generation;
	bool active;
	spinlock_t lock;
};

static rcu_st_node rcu_segtree[CONFIG_MAX_NR_CPUS * 2];

static uint64_t global_current_generation = 1;

static RCUHead *callback_list[4];

/**
 * rcu_call - defer a function call until every CPU has quiesced.
 * @head: RCU callback head structure
 * @function: callback function
 */
void
rcu_call (RCUHead *head, RCUCallback function)
{
	head->function = function;

	rcu_read_lock ();
	uint64_t gen = atomic_load_relaxed (&global_current_generation) + 1;
	RCUHead *next = atomic_load_relaxed (&callback_list[gen & 3]);
	for (;;) {
		head->next = next;
		bool ret = atomic_cmpxchg_weak (&callback_list[gen & 3],
				&next, head, mo_release, mo_relaxed);

		if (ret)
			break;
	}
	rcu_read_unlock ();
}

static DEFINE_PERCPU(DPC, rcu_cb_dispatch_dpc);
static DEFINE_PERCPU(RCUHead *, rcu_cb_dispatch_ptr);

static void
rcu_dispatch_dpc_func (DPC *dpc, void *arg1, void *arg2)
{
	(void) dpc;
	(void) arg1;
	(void) arg2;

	RCUHead *head = percpu_read (rcu_cb_dispatch_ptr);
	percpu_write (rcu_cb_dispatch_ptr, (RCUHead *) nullptr);

	while (head) {
		RCUHead *next = head->next;
		head->function (head);
		head = next;
	}
}

PERCPU_CONSTRUCTOR(rcu_dpc)
{
	DPC *dpc = percpu_ptr (rcu_cb_dispatch_dpc).on (cpu);
	RCUHead **head = percpu_ptr (rcu_cb_dispatch_ptr).on (cpu);

	dpc->init (rcu_dispatch_dpc_func, nullptr, nullptr);
	*head = nullptr;
}

static void
dispatch_callbacks (uint64_t generation)
{
	RCUHead *head = atomic_exchange (&callback_list[generation & 3],
			nullptr, mo_acquire);

	if (!head)
		return;

	if (percpu_read (rcu_cb_dispatch_ptr) != nullptr)
		panic ("RCU: dispatch_callbacks: old callbacks still haven't been dispatched!");

	percpu_write (rcu_cb_dispatch_ptr, head);
	DPC *dpc = percpu_ptr (rcu_cb_dispatch_dpc);
	dpc->enqueue ();
}

/**
 * rcu_begin_next_generation - begin a new RCU generation.
 * @old_generation: index of the previous generation
 */
static void
rcu_begin_next_generation (uint64_t old_generation)
{
	uint64_t new_generation = old_generation + 1;
	atomic_store_relaxed (&global_current_generation, new_generation);
	dispatch_callbacks (old_generation);
}

/**
 * rcu_quiesce - inform RCU that the CPU is in a quiescent state.
 *
 * This is typically called by the scheduler.
 */
void
rcu_quiesce (void)
{
	uint64_t current_generation = atomic_load_relaxed (&global_current_generation);
	unsigned int index = CONFIG_MAX_NR_CPUS + this_cpu_id ();
	while (index > 1U) {
		unsigned int parent = index >> 1;
		unsigned int buddy = index ^ 1U;

		rcu_segtree[parent].lock.raw_lock ();
		if (rcu_segtree[index].generation == current_generation) {
			rcu_segtree[parent].lock.raw_unlock ();
			return;
		}

		rcu_segtree[index].generation = current_generation;

		bool buddy_active = rcu_segtree[buddy].active;
		uint64_t buddy_gen = rcu_segtree[buddy].generation;

		rcu_segtree[parent].lock.raw_unlock ();
		if (buddy_active && buddy_gen != current_generation)
			return;

		index = parent;
	}

	rcu_begin_next_generation (current_generation);
}

/**
 * rcu_enable - announce CPU participation in RCU.
 *
 * This is typically called when entering a kernel context from userspace, or
 * when leaving an idle CPU state.
 */
void
rcu_enable (void)
{
	uint64_t current_generation = atomic_load_relaxed (&global_current_generation);
	unsigned int index = CONFIG_MAX_NR_CPUS + this_cpu_id ();
	while (index > 1U) {
		unsigned int parent = index >> 1;
		unsigned int buddy = index ^ 1U;

		rcu_segtree[parent].lock.raw_lock ();
		if (rcu_segtree[index].active) {
			rcu_segtree[parent].lock.raw_unlock ();
			return;
		}

		rcu_segtree[index].generation = current_generation;
		rcu_segtree[index].active = true;
		if (rcu_segtree[buddy].active) {
			rcu_segtree[parent].lock.raw_unlock ();
			return;
		}

		rcu_segtree[parent].lock.raw_unlock ();
		index = parent;
	}

	rcu_begin_next_generation (current_generation);
}

/**
 * rcu_disable - announce that a CPU stops participating in RCU.
 *
 * This is typically called when returning from a kernel context into userspace,
 * or when entering an idle CPU state.
 */
void
rcu_disable (void)
{
	uint64_t current_generation = atomic_load_relaxed (&global_current_generation);
	unsigned int index = CONFIG_MAX_NR_CPUS + this_cpu_id ();
	while (index > 1U) {
		unsigned int parent = index >> 1;
		unsigned int buddy = index ^ 1U;

		rcu_segtree[parent].lock.raw_lock ();
		if (!rcu_segtree[index].active) {
			rcu_segtree[parent].lock.raw_unlock ();
			return;
		}

		rcu_segtree[index].active = false;
		if (rcu_segtree[buddy].active) {
			rcu_segtree[parent].lock.raw_unlock ();
			return;
		}

		rcu_segtree[parent].lock.raw_unlock ();
		index = parent;
	}

	rcu_begin_next_generation (current_generation);
}

