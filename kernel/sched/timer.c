/* SPDX-License-Identifier: MIT */
#include <davix/sched.h>
#include <davix/time.h>
#include <davix/printk.h>
#include <davix/avltree.h>

struct avltree timer_list cpulocal;
spinlock_t timer_list_lock cpulocal;

static int avl_cmp_timers(struct avlnode *a, struct avlnode *b)
{
	struct raw_timer *timer_a = container_of(a, struct raw_timer, node);
	struct raw_timer *timer_b = container_of(b, struct raw_timer, node);

	if(timer_b->expiry_at > timer_a->expiry_at)
		return 1;

	return 0;
}

void register_timer(struct raw_timer *timer)
{
	struct avltree *timers = cpulocal_address(timer->on_cpu, &timer_list);
	spinlock_t *lock = cpulocal_address(timer->on_cpu, &timer_list_lock);
	int flag = spin_acquire_irq(lock);

	timer->expired = 0;
	avl_insert(&timer->node, timers, avl_cmp_timers, NULL);

	spin_release_irq(lock, flag);
}

void deregister_timer(struct raw_timer *timer)
{
	spinlock_t *lock = cpulocal_address(timer->on_cpu, &timer_list_lock);
	int flag = spin_acquire_irq(lock);

	/*
	 * The 'timer->expired' flag may have changed since we last read it.
	 */
	if(!timer->expired) {
		struct avltree *timers =
			cpulocal_address(timer->on_cpu, &timer_list);

		__avl_delete(&timer->node, timers, NULL);
	}
	spin_release_irq(lock, flag);
}

void timer_interrupt(void)
{
	/*
	 * This is literally how we decide when tasks need to switch.
	 */
	set_task_flag(current_task(), TASK_NEED_RESCHED);

	struct avltree *timers = cpulocal_address(smp_self(), &timer_list);

	_spin_acquire(cpulocal_address(smp_self(), &timer_list_lock));

	struct avlnode *timer_node = avl_first(timers);
	while(timer_node) {
		struct raw_timer *timer =
			container_of(timer_node, struct raw_timer, node);

		if(timer->expiry_at > ns_since_boot())
			break;

		struct avlnode *next = avl_successor(timer_node);
		__avl_delete(timer_node, timers, NULL);

		timer->expired = 1;
		timer->fn(timer);

		timer_node = next;
	}

	_spin_release(cpulocal_address(smp_self(), &timer_list_lock));
}

static void sched_timer_expire(struct raw_timer *raw)
{
	struct sched_timer *timer = container_of(raw, struct sched_timer, raw);
	sched_wake(timer->task);
}

void create_sched_timer(struct sched_timer *timer, nsecs_t at)
{
	timer->raw.on_cpu = smp_self();
	timer->raw.expiry_at = at;
	timer->raw.fn = sched_timer_expire;
	timer->task = current_task();
	register_timer(&timer->raw);
}

bool sched_timer_wait(struct sched_timer *timer)
{
	if(timer->raw.expired) {
		struct task *me = current_task();
		set_task_state(me, TASK_RUNNING);
		clear_task_flag(me, TASK_GOING_TO_SLEEP);
		return 1;
	}
	schedule();
	return timer->raw.expired;
}

/*
 * Destroy a sched timer.
 */
void destroy_sched_timer(struct sched_timer *timer)
{
	deregister_timer(&timer->raw);
}
