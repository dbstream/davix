/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_SCHED_H
#define __DAVIX_SCHED_H

/*
 * Declarations for low-level scheduler routines.
 *
 * Some of these, such as sched_after_task_switch(), are intended for
 * architectures to use as callbacks.
 */

#include <davix/atomic.h>
#include <davix/smp.h>
#include <davix/spinlock.h>
#include <asm/task.h>

typedef bitwise unsigned int task_flags_t;
#define __TF(x) ((force task_flags_t) (1 << x))

#define TASK_IDLE __TF(0) /* The task is an idle task. */
#define TASK_GOING_TO_SLEEP __TF(0) /* Postpone sched_wake() */

/*
 * These are the possible states of an alive task.
 *
 * Tasks that are currently running or on a runqueue must be in the TASK_RUNNING
 * state, and tasks that are not running and are not on a runqueue must be in a
 * different state, but there are some exceptions to this, notably a task that
 * is just about to schedule(), has the TASK_GOING_TO_SLEEP bit set and is
 * running with interrupts disabled.
 */
typedef unsigned int task_state_t;

#define TASK_RUNNING 0
#define TASK_UNINTERRUPTIBLE 1
#define TASK_INTERRUPTIBLE 2
#define TASK_DEAD 3

struct runqueue {
	/*
	 * A linked list of tasks on the runqueue.
	 */
	struct list tasks;

	/*
	 * A spinlock that protects the above list.
	 */
	spinlock_t rq_lock;
};

#define COMM_LEN 16

struct task {
	/*
	 * Architecture-dependent task information, used to store, for example,
	 * a kernel stack pointer, FPU state, SSE/AVX state, etc..
	 */
	struct arch_task_info arch_task_info;

	/*
	 * Task flags, updated atomically using {set,clear}_task_flag().
	 */
	atomic task_flags_t flags;

	/*
	 * Task state, updated atomically using {set,read}_task_state().
	 */
	atomic task_state_t task_state;

	/*
	 * List entry in the runqueue.
	 */
	struct list rq_list;

	char comm[COMM_LEN];
};

static inline void set_task_flag(struct task *task, task_flags_t flag)
{
	atomic_fetch_or((force unsigned int *) &task->flags,
		(force unsigned int) flag, memory_order_seq_cst);
}

static inline void clear_task_flag(struct task *task, task_flags_t flag)
{
	atomic_fetch_and((force unsigned int *) &task->flags,
		(force unsigned int) ~flag, memory_order_seq_cst);
}

static inline bool test_task_flag(struct task *task, task_flags_t flag)
{
	return (atomic_load((force unsigned int *) &task->flags,
		memory_order_seq_cst) & (force unsigned int) flag) ? 1 : 0;
}

static inline void set_task_state(struct task *task, task_state_t task_state)
{
	atomic_store(&task->task_state, task_state, memory_order_seq_cst);
}

static inline task_state_t read_task_state(struct task *task)
{
	return atomic_load(&task->task_state, memory_order_seq_cst);
}

extern struct task *idle_task cpulocal;

extern struct task *__current_task cpulocal;

static inline struct task *current_task(void)
{
	return rdwr_cpulocal(__current_task);
}

static inline void set_current_task(struct task *task)
{
	rdwr_cpulocal(__current_task) = task;
}

/*
 * Perform architecture-specific initialization of an idle task.
 */
void arch_init_idle_task(struct logical_cpu *cpu, struct task *task);

/*
 * Perform architecture-specific initialization of the init task.
 */
void arch_setup_init_task(struct task *task);

/*
 * Perform the context switch to a new task.
 */
void switch_to(struct task *from, struct task *to);

/*
 * Initialize basic scheduler data structures on a particular CPU.
 */
void setup_sched_on(struct logical_cpu *cpu);

void sched_init(void);

/*
 * Architectures must call this after every task switch on the stack of the new
 * task switched to.
 */
void sched_after_task_switch(struct task *from, struct task *to);

/*
 * schedule() - switch task to the next task in the runqueue.
 *
 * This function, along with sched_after_task_switch() and sched_wake(), is the
 * "heart" of the scheduler.
 */
void schedule(void);

/*
 * Wake the task if it isn't already running.
 *
 * Return value: true if the task was woken up, otherwise false.
 */
bool sched_wake(struct task *task);

#endif /* __DAVIX_SCHED_H */
