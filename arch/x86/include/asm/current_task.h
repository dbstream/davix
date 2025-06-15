/**
 * Architecture implementation of get_current_task.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

struct Task;

static inline Task *
get_current_task (void)
{
	Task *ret;
	asm volatile ("movq %%gs:24, %0" : "=r" (ret));
	return ret;
}

static inline void
set_current_task (Task *task)
{
	asm volatile ("movq %0, %%gs:24" :: "r" (task));
}

