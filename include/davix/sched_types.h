/**
 * Scheduler types.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_SCHED_TYPES_H
#define _DAVIX_SCHED_TYPES_H 1

#include <davix/list.h>
#include <asm/task.h>

struct process_mm;

struct rq_entry {
	struct list list;
};

struct task {
	/**
	 * Architecture-specific task state; must be the first member.
	 */
	struct arch_task_state arch;

	/**
	 * Task flags. Immutable after task creation.
	 */
	unsigned int flags;

	/**
	 * Task state. This must only be accessed on the CPU that the task
	 * belongs to, either in IRQ context or when IRQs are disabled.
	 * Otherwise races can occur.
	 */
	unsigned int state;

	/**
	 * This is the processor that the task belongs to.
	 */
	unsigned int cpu;

	/**
	 * rq_entry: only used when the task is on the runqueue.
	 */
	struct rq_entry rq_entry;

	char comm[16];

	/**
	 * This is the memory manager instance of the task (or NULL for kernel
	 * tasks).
	 */
	struct process_mm *mm;

	/**
	 * This is the filesystem context of the task.  (or NULL for kernel
	 * tasks)
	 */
	struct fs_context *fs;
};

#define TASK_RUNNABLE 0
#define TASK_INTERRUPTIBLE 1
#define TASK_UNINTERRUPTIBLE 2
#define TASK_ZOMBIE 3

#define TF_IDLE 1U		/** The task is an idle task.  */
#define TF_NOMIGRATE 2U		/** The task is to be excluded form migration.  */

#endif /** _DAVIX_SCHED_TYPES_H  */
