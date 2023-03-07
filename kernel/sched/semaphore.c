#include <davix/semaphore.h>
#include <davix/sched.h>
#include <davix/errno.h>
#include <davix/panic.h>
#include <davix/list.h>

int sem_try_wait(struct semaphore *sem)
{
	spin_acquire(&sem->lock);

	int ret;
	if(sem->count) {
		sem->count--;
		ret = 1;
	} else {
		ret = 0;
	}

	spin_release(&sem->lock);
	return ret;
}

void sem_wait(struct semaphore *sem)
{
	spin_acquire(&sem->lock);
	if(sem->count) {
		sem->count--;
		spin_release(&sem->lock);
		return;
	}

	struct task *me = current_task();
	struct semaphore_waiter waiter = {
		.task = me,
		.flag = 0
	};
	list_add(&sem->waiters, &waiter.list);
	set_task_flag(me, TASK_GOING_TO_SLEEP);
	set_task_state(me, TASK_UNINTERRUPTIBLE);
	_spin_release(&sem->lock); /* keep preempt disabled */
	schedule();
	if(!waiter.flag)
		panic("sem_wait(): \"%s\" woke up, but didn't acquire semaphore",
			me->comm);
	preempt_enable();
}

int sem_wait_timeout(struct semaphore *sem, nsecs_t timeout)
{
	int ret = 0;
	spin_acquire(&sem->lock);
	if(sem->count) {
		sem->count--;
		goto out;
	}

	struct sched_timer timer;
	create_sched_timer(&timer, ns_since_boot() + timeout);

	struct task *me = current_task();
	struct semaphore_waiter waiter = {
		.task = me,
		.flag = 0
	};
	list_add(&sem->waiters, &waiter.list);

	set_task_flag(me, TASK_GOING_TO_SLEEP);
	set_task_state(me, TASK_UNINTERRUPTIBLE);

	_spin_release(&sem->lock);
	sched_timer_wait(&timer);
	_spin_acquire(&sem->lock);

	if(!waiter.flag) {
		list_del(&waiter.list);
		ret = ETIME;
	}

	destroy_sched_timer(&timer);
out:
	spin_release(&sem->lock);
	return ret;
}

void sem_signal(struct semaphore *sem)
{
	spin_acquire(&sem->lock);
	if(list_empty(&sem->waiters)) {
		sem->count++;
		spin_release(&sem->lock);
		return;
	}

	struct semaphore_waiter *to_wake =
		container_of(sem->waiters.next, struct semaphore_waiter, list);

	list_del(&to_wake->list);
	to_wake->flag = 1;
	spin_release(&sem->lock);
	sched_wake(to_wake->task);
}
