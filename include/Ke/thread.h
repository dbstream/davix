// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/thread.h
 * KTHREAD
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Hal/thread.h>
#include <Ke/time.h>
#include <OS/status.h>
#include <dsl/avltree.h>
#include <dsl/list.h>

struct KSCHEDULER;

struct KTHREAD {
	HalThreadData hal;

	int thread_state;

	bool running;

	unsigned int active_wakeup_count;

	union {
		dsl::AVLNode rq_avl;
		dsl::ListHead rq_list;
	};

	unsigned long long vruntime;
	nsec_t last_vruntime_update;

	int base_priority;
	int current_priority;


	char comm[32];
};

enum : int {
	/*
	 * KTHREAD_RUNNING: the thread is running.
	 */
	KTHREAD_RUNNING			= 0,
	/*
	 * KTHREAD_INTERRUPTIBLE: the thread is sleeping but can be woken up by
	 * pending signals.
	 */
	KTHREAD_INTERRUPTIBLE		= 1,
	/*
	 * KTHREAD_KILLABLE: the thread is sleeping but can be woken up by
	 * OS_SIGNAL_KILL.
	 */
	KTHREAD_KILLABLE		= 2,
	/*
	 * KTHREAD_UNINTERRUPTIBLE: the thread is sleeping and cannot be woken
	 * up by pending signals.
	 */
	KTHREAD_UNINTERRUPTIBLE		= 3,
	/*
	 * KTHREAD_WAKING_UP: transient state used internally by the scheduler
	 * when a thread is waking up.
	 */
	KTHREAD_WAKING_UP		= 4,
	/*
	 * KTHREAD_ZOMBIE: the thread has died.
	 */
	KTHREAD_ZOMBIE			= 5,
};

static constexpr int KPRIORITY_MIN = 0;
static constexpr int KPRIORITY_MAX = 10;
static constexpr int KPRIORITY_MAX_CFS = 9;
static constexpr int KPRIORITY_REALTIME = 10;

OSSTATUS KeInitializeThread(
	KTHREAD *thread,
	void (*entrypoint)(void *),
	void *arg
);

void KeDestroyThread(KTHREAD *thread);

void KeInitializeIdleThread(KTHREAD *thread);

void KeSetThreadComm(KTHREAD *thread, const char *comm);

void KeSetCurrentState(int state);

