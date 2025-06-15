/**
 * Architecture-specific task data.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

struct Task;
struct task_switch_frame;

struct arch_task_info {
	struct task_switch_frame *stack_pointer;
	void *stack_bottom;
};

bool
arch_create_task (Task *task, void (*entry_function)(void *), void *arg);

void
arch_free_task (Task *task);

