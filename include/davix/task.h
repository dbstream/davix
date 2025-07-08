/**
 * struct Task
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <asm/task.h>
#include <davix/path.h>
#include <davix/time.h>
#include <dsl/list.h>
#include <stdint.h>

#ifndef SCHED_TICKET_T_DEFINED
typedef uint64_t sched_ticket_t;
#endif

enum : int {
	MIN_TASK_PRIORITY = 0,
	MAX_TASK_PRIORITY = 20
};

struct Task {
	arch_task_info arch;

	int task_state;
	unsigned int task_flags;

	dsl::ListHead rq_list_entry;

	int base_priority;
	int current_priority;

	sched_ticket_t unblock_ticket;

	int pending_wakeup;
	unsigned int on_cpu;

	unsigned int last_cpu;

	FSContext *ctx_fs;

	char comm[16];
};

Task *
alloc_task_struct (void);

void
free_task_struct (Task *tsk);

void
init_task_struct_fields (Task *tsk);

void
reap_task (Task *tsk);
