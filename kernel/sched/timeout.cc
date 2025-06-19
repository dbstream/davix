/**
 * sched_timeout() - sleep until timer expires or something else wakes you up.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/irql.h>
#include <asm/percpu.h>
#include <davix/atomic.h>
#include <davix/ktimer.h>
#include <davix/sched.h>
#include <davix/spinlock.h>
#include <davix/time.h>
#include <dsl/avltree.h>

struct timeout_cpu_data;

static constexpr nsecs_t expire_never = -1ULL;

struct sched_timeout_struct {
	dsl::AVLNode tree_entry;
	nsecs_t expiry;
	Task *task;
	sched_ticket_t ticket;
	bool removed;
	timeout_cpu_data *td;
};

struct sched_timer_comparator {
	constexpr bool
	operator() (const sched_timeout_struct *lhs,
			const sched_timeout_struct *rhs) const
	{
		return lhs->expiry < rhs->expiry;
	}
};

typedef dsl::TypedAVLTree<sched_timeout_struct,
		&sched_timeout_struct::tree_entry,
		sched_timer_comparator> TimeoutTree;

struct timeout_cpu_data {
	KTimer timer;
	TimeoutTree tree;
	nsecs_t timer_expiry;
	spinlock_t lock;	/* so remote processors can dequeue entries  */
};

static DEFINE_PERCPU(timeout_cpu_data, timeout_data);

static void
handle_timer_event (KTimer *tmr, void *arg);

PERCPU_CONSTRUCTOR(timeout_cpu_data)
{
	timeout_cpu_data *td = percpu_ptr (timeout_data).on (cpu);
	td->timer.init (handle_timer_event, nullptr);
	td->tree.init ();
	td->timer_expiry = expire_never;
	td->lock.init ();
}

static void
handle_timer_event (KTimer *tmr, void *arg)
{
	(void) arg;

	timeout_cpu_data *td = container_of(&timeout_cpu_data::timer, tmr);

	/* NB: we are already in DPC context.  */
	td->lock.raw_lock ();
	td->timer_expiry = 0;
	for (;;) {
		if (td->tree.empty ()) {
			td->timer_expiry = expire_never;
			break;
		}

		sched_timeout_struct *entry = td->tree.first ();
		nsecs_t expiry = entry->expiry;
		if (expiry > ns_since_boot ()) {
			if (expiry < expire_never)
				td->timer.enqueue (expiry);
			td->timer_expiry = expiry;
			break;
		}

		td->tree.remove (entry);
		atomic_store_relaxed (&entry->removed, true);
		Task *tsk = entry->task;
		sched_ticket_t ticket = entry->ticket;
		td->lock.raw_unlock ();

		sched_wake (tsk, ticket);

		td->lock.raw_lock ();
	}
	td->lock.raw_unlock ();
}

void
sched_timeout_ticket (nsecs_t expiry, int state, sched_ticket_t ticket)
{
	sched_timeout_struct entry;
	entry.expiry = expiry;
	entry.task = get_current_task ();
	entry.ticket = ticket;
	entry.removed = false;

	disable_dpc ();
	timeout_cpu_data *td = percpu_ptr (timeout_data);
	entry.td = td;

	td->lock.raw_lock ();
	td->tree.insert (&entry);
	if (expiry < td->timer_expiry) {
		if (td->timer_expiry != expire_never)
			td->timer.remove ();
		td->timer.enqueue (expiry);
		td->timer_expiry = expiry;
	}

	if (state)
		set_current_state (state);

	td->lock.raw_unlock ();
	schedule ();

	if (atomic_load_relaxed (&entry.removed)) {
		enable_dpc ();
		return;
	}

	td->lock.raw_lock ();
	if (!entry.removed)
		td->tree.remove (&entry);
	td->lock.raw_unlock ();
	enable_dpc ();
}

void
sched_timeout (nsecs_t expiry, int state)
{
	sched_timeout_ticket (expiry, state, sched_get_blocking_ticket ());
}

