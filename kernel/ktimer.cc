/**
 * ktimer subsystem
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/percpu.h>
#include <davix/dpc.h>
#include <davix/irql.h>
#include <davix/ktimer.h>

static constexpr nsecs_t expire_never = -1ULL;

static void
timer_dpc_func (DPC *dpc, void *arg1, void *arg2);

struct KTimerCmp {
	constexpr bool
	operator() (const KTimer *lhs, const KTimer *rhs) const
	{
		return lhs->expiry_ns < rhs->expiry_ns;
	}
};

typedef dsl::TypedAVLTree<KTimer, &KTimer::tree_node, KTimerCmp> KTimerTree;

struct KTimerQueue {
	KTimerTree tree;	/** NOT safe to use in IRQ context.  */
	nsecs_t next_expiry;	/** Safe to use in IRQ context.  */
	DPC timer_dpc;
};

static DEFINE_PERCPU(KTimerQueue, globalKtimerQueue);

PERCPU_CONSTRUCTOR(kernel_ktimer)
{
	KTimerQueue *queue = percpu_ptr (globalKtimerQueue).on (cpu);
	queue->tree.init ();
	queue->next_expiry = expire_never;
	queue->timer_dpc.init (timer_dpc_func, nullptr, nullptr);
}

/**
 * ktimer_handle_timer_interrupt - handle the local timer interrupt.
 */
void
ktimer_handle_timer_interrupt (void)
{
	nsecs_t next_expiry = percpu_read (globalKtimerQueue.next_expiry);
	nsecs_t now = ns_since_boot ();

	if (now >= next_expiry) {
		percpu_write (globalKtimerQueue.next_expiry, expire_never);
		DPC *dpc = percpu_ptr (globalKtimerQueue.timer_dpc);
		dpc->enqueue ();
	}
}

static void
timer_dpc_func (DPC *dpc, void *arg1, void *arg2)
{
	(void) dpc;
	(void) arg1;
	(void) arg2;

	KTimerQueue *queue = percpu_ptr (globalKtimerQueue);
	nsecs_t now = ns_since_boot ();
	for (;;) {
		KTimer *timer = queue->tree.first ();
		if (!timer)
			return;

		nsecs_t t = timer->expiry_ns;
		if (now < t)
			now = ns_since_boot ();
		if (now < t) {
			disable_irq ();
			if (t < queue->next_expiry)
				queue->next_expiry = t;
			enable_irq ();
			return;
		}

		queue->tree.remove (timer);
		timer->on_queue = false;
		timer->callback_fn (timer, timer->callback_arg);
	}
}

bool
KTimer::enqueue (nsecs_t t)
{
	scoped_dpc g;

	if (on_queue)
		return false;

	on_queue = true;
	expiry_ns = t;

	KTimerQueue *queue = percpu_ptr (globalKtimerQueue);

	disable_irq ();
	if (t < queue->next_expiry)
		queue->next_expiry = t;
	enable_irq ();
	queue->tree.insert (this);
	return true;
}

bool
KTimer::remove (void)
{
	scoped_dpc g;

	if (!on_queue)
		return false;

	on_queue = false;

	KTimerQueue *queue = percpu_ptr (globalKtimerQueue);
	queue->tree.remove (this);

	disable_irq ();
	if (expiry_ns == queue->next_expiry) {
		enable_irq ();
		KTimer *first = queue->tree.first ();
		nsecs_t t = first ? first->expiry_ns : expire_never;
		disable_irq ();
		queue->next_expiry = t;
	}
	enable_irq ();
	return true;
}
