// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/thread.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ke/sched.h>
#include <Ke/thread.h>
#include <string.h>

static void thread_timed_out(KSYSTIMER *timer)
{
	KTHREAD *thread = container_of(&KTHREAD::timeout_systimer, timer);

	KeWakeThread(thread);
}

OSSTATUS KeInitializeThread(
	KTHREAD *thread,
	void (*entrypoint)(void *),
	void *arg
)
{
	thread->thread_state = KTHREAD_UNINTERRUPTIBLE;
	thread->running = false;
	thread->active_wakeup_count = 0;
	thread->vruntime = 0;
	thread->last_vruntime_update = 0;
	thread->base_priority = KPRIORITY_MIN;
	thread->current_priority = KPRIORITY_MIN;
	thread->sleep_timeout = 0;
	thread->timeout_systimer.init(thread_timed_out);
	KeSetThreadComm(thread, "(uninitialized)");

	OSSTATUS status = HalInitializeThread(&thread->hal, entrypoint, arg);
	if (!OS_SUCCESS(status))
		return status;

	return OS_STATUS_SUCCESS;
}

void KeDestroyThread(KTHREAD *thread)
{
	HalDestroyThread(&thread->hal);
}

void KeInitializeIdleThread(KTHREAD *thread)
{
	thread->thread_state = KTHREAD_RUNNING;
	thread->running = true;
	thread->active_wakeup_count = 0;
	thread->vruntime = 0;
	thread->last_vruntime_update = 0;
	thread->base_priority = KPRIORITY_REALTIME;
	thread->current_priority = KPRIORITY_REALTIME;
	KeSetThreadComm(thread, "idle");

	HalInitializeIdleThread(&thread->hal);
}

void KeSetThreadComm(KTHREAD *thread, const char *comm)
{
	bzero(thread->comm, sizeof(thread->comm));
	strncpy(thread->comm, comm, sizeof(thread->comm) - 1);
}

