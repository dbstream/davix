/**
 * Davix scheduler
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

enum : int {
	TASK_RUNNABLE		= 0,
	TASK_INTERRUPTIBLE	= 1,
	TASK_UNINTERRUPTIBLE	= 2,
	TASK_ZOMBIE		= 3
};

enum : unsigned int {
	TF_IDLE			= 1U,	/* The task is an idle task.  */
	TF_NOMIGRATE		= 2U,	/* The task must stay on a single CPU.  */
};

void
sched_idle (void);
