/**
 * Timer events.
 * Copyright (C) 2024  dbstream
 */
#include <davix/avltree.h>
#include <davix/stddef.h>
#include <davix/panic.h>
#include <davix/timer.h>
#include <asm/cpulocal.h>
#include <asm/irq.h>
#include <asm/smp.h>

static struct avl_tree ktimer_queue __CPULOCAL;

static inline int
ktimer_compare (struct avl_node *node1, struct avl_node *node2)
{
	struct ktimer *t1 = struct_parent (struct ktimer, node, node1);
	struct ktimer *t2 = struct_parent (struct ktimer, node, node2);

	return t2->expiry > t1->expiry ? 1 : 0;
}

void
ktimer_add (struct ktimer *timer)
{
	int cpu = this_cpu_id ();

	timer->cpu = cpu;
	timer->triggered = false;

	bool flag = irq_save ();
	avl_insert (this_cpu_ptr (&ktimer_queue), &timer->node, ktimer_compare);
	irq_restore (flag);
}

bool
ktimer_remove (struct ktimer *timer)
{
	if (this_cpu_id () != timer->cpu)
		panic ("sched: ktimer_remove was called on a different CPU from the timer CPU\n");

	bool flag = irq_save ();

	bool ret = timer->triggered;
	if (!ret)
		__avl_remove (this_cpu_ptr (&ktimer_queue), &timer->node);

	irq_restore (flag);
	return ret;
}

void
local_timer_tick (void)
{
	struct avl_tree *tree = this_cpu_ptr (&ktimer_queue);
	while (tree->root) {
		struct avl_node *node = tree->root;
		while (avl_left (node))
			node = avl_left (node);

		struct ktimer *t = struct_parent (struct ktimer, node, node);
		if (ns_since_boot () < t->expiry)
			break;

		__avl_remove (tree, node);
		t->triggered = true;
		t->callback (t);
	}
}
