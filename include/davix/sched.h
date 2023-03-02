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

struct mm;

typedef bitwise unsigned int task_flags_t;
#define __TF(x) ((force task_flags_t) (1 << x))

#define TASK_IDLE __TF(0) /* The task is an idle task. */
#define TASK_GOING_TO_SLEEP __TF(1) /* Postpone sched_wake() */
#define TASK_NEED_RESCHED __TF(2)

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

	/*
	 * Most likely the first 15 chars of what was passed to execve().
	 */
	char comm[COMM_LEN];

	/*
	 * VMM for the task.
	 * NOTE: The task holds a reference to this. Therefore, the CPU that is
	 * actively using this MM does not need to hold a reference to it.
	 */
	struct mm *mm;
};

/*
 * Reschedule if it is needed.
 * This is called on kernel->user return, IRQ return and
 * outermost preempt-enables.
 */
void maybe_resched(void);

extern unsigned long preempt_disabled cpulocal;

static inline void preempt_disable(void)
{
	bool irqs = interrupts_enabled();
	disable_interrupts();

	rdwr_cpulocal(preempt_disabled)++;

	if(irqs)
		enable_interrupts();
}

static inline void preempt_enable(void)
{
	rdwr_cpulocal(preempt_disabled)--;
	maybe_resched();
}

static inline bool preempt_enabled(void)
{
	bool irqs = interrupts_enabled();
	disable_interrupts();

	unsigned long value = rdwr_cpulocal(preempt_disabled);

	if(irqs)
		enable_interrupts();

	return value == 0;
}

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

/*
 * Safely read the current task.
 *
 * NOTE: Using this should be avoided from within scheduler code, because
 * it has a preempt_{disable/enable} pair, which might lead to the scheduler
 * calling into itself, thereby leading to infinite recursion.
 */
static inline struct task *current_task(void)
{
	preempt_disable();
	struct task *ret = rdwr_cpulocal(__current_task);
	preempt_enable();
	return ret;
}

/*
 * Perform architecture-specific initialization of an idle task.
 */
void arch_init_idle_task(struct logical_cpu *cpu, struct task *task);

/*
 * Setup architecture-specific state of a task that will cause it to call
 * 'function' with 'arg'.
 */
int arch_setup_task(struct task *task, void (*function)(void *), void *arg);

/*
 * Perform architecture-specific initialization of the init task.
 */
void arch_setup_init_task(struct task *task);

/*
 * Free architecture-specific task resources.
 */
void arch_task_died(struct task *task);

/*
 * Perform the context switch to a new task.
 */
void switch_to(struct task *from, struct task *to);

void sched_init(void);

/*
 * Architectures must call this after every task switch on the stack of the new
 * task switched to.
 */
void sched_after_task_switch(struct task *from, struct task *to);

/*
 * Architectures must call this function as the first thing they call in a new
 * task.
 */
void sched_after_fork(void);

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

void handle_dead_task(struct task *task);

void sched_start_task(struct task *task);
int create_kernel_task(const char *name, void (*function)(void *), void *arg);

#endif /* __DAVIX_SCHED_H */
