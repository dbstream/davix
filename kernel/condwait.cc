/**
 * Wait-on-condition: wait for some condition on an object to become true.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/condwait.h>
#include <davix/sched.h>
#include <davix/spinlock.h>
#include <dsl/hlist.h>
#include <uapi/davix/errno.h>

struct condwait_waiter {
	dsl::HListHead list;
	Task *task;
	sched_ticket_t ticket;
	int on_list;
};

typedef dsl::TypedHList<condwait_waiter, &condwait_waiter::list> CondWaiterList;

static constexpr uintptr_t condwait_hash_bits = 0xff;

static struct condwait_bucket {
	CondWaiterList list;
	spinlock_t lock;
} static_condwait_buckets[condwait_hash_bits + 1];

static inline condwait_bucket *
get_bucket (condwait_key_t key)
{
	return static_condwait_buckets + ((uintptr_t) key & condwait_hash_bits);
}

condwait_key_t
condwait_obj_key (const void *obj)
{
	uintptr_t x = (uintptr_t) obj;
	x ^= x << 7;
	x ^= x >> 9;
	return (condwait_key_t) x;
}

void
__condwait_touch (condwait_key_t key)
{
	condwait_bucket *bucket = get_bucket (key);

	bucket->lock.lock_dpc ();
	while (!bucket->list.empty ()) {
		condwait_waiter *waiter = bucket->list.pop ();
		atomic_store_release (&waiter->on_list, 0);
		sched_wake (waiter->task, waiter->ticket);
	}
	bucket->lock.unlock_dpc ();
}

int
__cond_wait_on (condwait_key_t key, bool (*cond) (const void *), const void *arg,
		bool interruptible, nsecs_t timeout)
{
	int state = interruptible ? TASK_INTERRUPTIBLE : TASK_UNINTERRUPTIBLE;
	condwait_bucket *bucket = get_bucket (key);
	condwait_waiter w;
	w.task = get_current_task ();

	if (timeout != NO_TIMEOUT)
		timeout += ns_since_boot ();

retry:
	bucket->lock.lock_dpc ();
	if (cond (arg)) {
		bucket->lock.unlock_dpc ();
		return 0;
	}

	w.ticket = sched_get_blocking_ticket ();
	w.on_list = 1;
	bucket->list.push (&w);
	bucket->lock.raw_unlock ();
	sched_timeout (timeout, state);

	if (atomic_load_acquire (&w.on_list)) {
		bucket->lock.raw_lock ();
		if (w.on_list)
			w.list.remove ();
		bucket->lock.unlock_dpc ();
	} else
		enable_dpc ();

	if (cond (arg))
		return 0;
	if (ns_since_boot () >= timeout)
		return ETIME;
	if (has_pending_signal ())
		return EINTR;
	goto retry;
}
