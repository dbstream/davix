/**
 * Kernel workqueues
 * Copyright (C) 2024  dbstream
 */
#include <davix/context.h>
#include <davix/panic.h>
#include <davix/sched.h>
#include <davix/snprintf.h>
#include <davix/task_api.h>
#include <davix/workqueue.h>
#include <asm/cpulocal.h>
#include <asm/smp.h>
#include <davix/printk.h>

static __CPULOCAL struct task *worker_task;

static __CPULOCAL struct list worker_work_list;

static void
kworker (void *arg)
{
	(void) arg;

	sched_begin_task ();

	struct task *me = get_current_task ();
	struct list *work_list = this_cpu_ptr (&worker_work_list);

	irq_disable ();
	for (;;) {
		while (!list_empty (work_list)) {
			struct work *item = list_item (struct work, list, work_list->next);
			list_delete (&item->list);
			irq_enable ();
			item->execute (item);
			irq_disable ();
		}

		me->state = TASK_INTERRUPTIBLE;
		schedule ();
	}
}

void
workqueue_init_this_cpu (void)
{
	char name[16];
	snprintf (name, sizeof (name), "kworker-cpu%u", this_cpu_id ());

	list_init (this_cpu_ptr (&worker_work_list));
	struct task *worker = create_kernel_task (name, kworker, NULL, TF_NOMIGRATE);
	if (!worker)
		panic ("workqueue: could not create %s", name);

	this_cpu_write (&worker_task, worker);
}

void
wq_enqueue_work (struct work *work)
{
	preempt_off ();
	list_insert_back (this_cpu_ptr (&worker_work_list), &work->list);
	sched_wake__this_cpu (this_cpu_read (&worker_task));
	preempt_on ();
}
