// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ex/thread.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ex/thread.h>
#include <Ke/log.h>
#include <Mm/pool.h>
#include <davix/vsnprintf.h>

OSSTATUS ExCreateThread(ETHREAD **out, void (*entrypoint)(void *), void *arg)
{
	ETHREAD *thread = MmNew<ETHREAD>();
	if (!thread)
		return OS_STATUS_KERNEL_ALLOCATION_FAILED;

	OSSTATUS status = KeInitializeThread(&thread->thread, entrypoint, arg);
	if (!OS_SUCCESS(status)) {
		MmDelete(thread);
		return status;
	}

	*out = thread;
	return OS_STATUS_SUCCESS;
}

void ExDestroyThread(ETHREAD *thread)
{
	KeDestroyThread(&thread->thread);
	MmDelete(thread);
}

ETHREAD *ExCreateIdleThread(unsigned int cpu)
{
	ETHREAD *thread = MmNew<ETHREAD>();
	if (!thread)
		KePanic("ExCreateIdleThread: out of memory!");

	KeInitializeIdleThread(&thread->thread);

	char comm[32];
	snprintf(comm, sizeof(comm), "idle-%u", cpu);
	KeSetThreadComm(&thread->thread, comm);

	return thread;
}

