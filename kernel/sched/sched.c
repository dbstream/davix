/* SPDX-License-Identifier: MIT */
#include <davix/sched.h>
#include <davix/kmalloc.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/printk_lib.h>
#include <davix/list.h>

struct task *idle_task cpulocal;
struct task *__current_task cpulocal;

static struct task init_task;

static struct runqueue runqueue = {
	.tasks = LIST_INIT(runqueue.tasks),
	.rq_lock = SPINLOCK_INIT(runqueue.rq_lock)
};

static inline struct task *get_task_from_runqueue(void)
{
	struct task *ret = NULL;
	_spin_acquire(&runqueue.rq_lock);
	if(!list_empty(&runqueue.tasks)) {
		ret = container_of(runqueue.tasks.next, struct task, rq_list);
		list_del(&ret->rq_list);
	}
	_spin_release(&runqueue.rq_lock);
	return ret;
}

static inline void add_task_to_runqueue(struct task *task)
{
	int irqflag = spin_acquire(&runqueue.rq_lock);
	list_radd(&runqueue.tasks, &task->rq_list);
	spin_release(&runqueue.rq_lock, irqflag);
}

static void setup_sched_on(struct logical_cpu *cpu)
{
	struct task *idle = kmalloc(sizeof(struct task));
	if(!idle)
		panic("setup_sched_on(CPU#%u): couldn't allocate memory for idle task.",
			cpu->id);

	idle->flags = TASK_IDLE;
	idle->task_state = TASK_RUNNING;

	snprintk(idle->comm, COMM_LEN, "idle-%u\n", cpu->id);

	arch_init_idle_task(cpu, idle);
	rdwr_cpulocal(idle_task) = idle;
}

void sched_init(void)
{
	for_each_logical_cpu(cpu) {
		if(!cpu->possible)
			return;
		setup_sched_on(cpu);
	}
	init_task.flags = 0;
	init_task.task_state = TASK_RUNNING;
	strncpy(init_task.comm, "init", COMM_LEN);
	arch_setup_init_task(&init_task);
	set_current_task(&init_task);
}

static void handle_dead_task(struct task *task)
{
	info("sched: Task \"%.*s\" (%p) died.\n", COMM_LEN, task->comm, task);
}

void sched_after_task_switch(struct task *from, struct task *to)
{
	set_current_task(to);

	clear_task_flag(from, TASK_GOING_TO_SLEEP);

	task_state_t old_task_state = read_task_state(from);
	if(old_task_state == TASK_RUNNING) {
		/*
		 * Don't place the idle task(s) on the runqueue.
		 */
		if(!test_task_flag(from, TASK_IDLE))
			add_task_to_runqueue(from);
	} else if(old_task_state == TASK_DEAD) {
		handle_dead_task(from);
	}

	/*
	 * TASK_(UN)INTERRUPTIBLE doesn't need any special handling.
	 */
}

bool sched_wake(struct task *task)
{
	while(test_task_flag(task, TASK_GOING_TO_SLEEP))
		relax();

	task_state_t state = read_task_state(task);
	if(state == TASK_DEAD) {
		warn("sched_wake(): Trying to wake dead task \"%.*s\" (%p)\n",
			COMM_LEN, task->comm, task);
		return 0;
	}

	if(state == TASK_RUNNING)
		return 0;

	if(!atomic_compare_exchange_strong(&task->task_state, &state,
		TASK_RUNNING, memory_order_seq_cst, memory_order_seq_cst))
			return 0;

	add_task_to_runqueue(task);
	return 1;
}

void schedule(void)
{
	int irqflag = interrupts_enabled();
	disable_interrupts();

	struct task *me = current_task();
	struct task *task = get_task_from_runqueue();

	if((read_task_state(me) != TASK_RUNNING))
		task = rdwr_cpulocal(idle_task);

	if(task)
		switch_to(me, task);

	if(irqflag)
		enable_interrupts();
}
