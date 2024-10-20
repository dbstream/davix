/**
 * Operations on runqueues.
 * Copyright (C) 2024  dbstream
 *
 * This file implements the scheduler semantics.
 *
 * note: Accounting (e.g. num_runnable) is done by main.
 */
#include <davix/sched.h>
#include "internal.h"

/**
 * Currently, we implement a trivial round-robin scheduler. Each runqueue
 * is a FIFO list of tasks.
 */

void
rq_init (struct runqueue *rq)
{
	list_init (&rq->runnable_list);
}

bool
rq_enqueue (struct task *task)
{
	struct runqueue *rq = this_cpu_runqueue ();

	list_insert_back (&rq->runnable_list, &task->rq_entry.list);
	return rq->runnable_list.next == rq->runnable_list.prev;
}

bool
rq_dequeue (struct task *task)
{
	struct runqueue *rq = this_cpu_runqueue ();

	list_delete (&task->rq_entry.list);
	return list_empty (&rq->runnable_list);
}

struct task *
rq_choose_next (void)
{
	struct runqueue *rq = this_cpu_runqueue ();
	if (list_empty (&rq->runnable_list))
		return NULL;

	struct task *task = list_item (struct task,
		rq_entry.list, rq->runnable_list.next);

	list_delete (&task->rq_entry.list);
	return task;
}

nsecs_t
rq_get_deadline (void)
{
	struct runqueue *rq = this_cpu_runqueue ();
	if (list_empty (&rq->runnable_list))
		return 0;

	/* Give each task a 10ms timeslice. */
	return rq->last_task_switch + 10000000;
}
