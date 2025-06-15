/**
 * Davix scheduler
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <asm/current_task.h>
#include <stdint.h>

struct Task;

#ifndef SCHED_TICKET_T_DEFINED
typedef uint64_t sched_ticket_t;
#endif

enum : sched_ticket_t {
	SCHED_WAKE_INITIAL = 0
};

enum : int {
	TASK_RUNNABLE		= 0,
	TASK_INTERRUPTIBLE	= 1,
	TASK_UNINTERRUPTIBLE	= 2,
	TASK_ZOMBIE		= 3
};

enum : unsigned int {
	TF_IDLE			= 1U,	/* The task is an idle task.  */
	TF_NOMIGRATE		= 2U,	/* The task must stay on a single CPU.  */
	TF_INTERRUPT		= 4U,	/* The task is an interrupt thread.  */
};

void
sched_idle (void);

void
set_current_state (int state);

sched_ticket_t
sched_get_blocking_ticket (void);

bool
sched_wake (Task *task, sched_ticket_t ticket);

void
schedule (void);

bool
has_pending_signal (void);

void
sched_init (void);

void
sched_init_this_cpu (void);

void
handle_reschedule_IPI (void);

void
finish_context_switch (Task *prev);
