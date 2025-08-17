// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ex/thread.h
 * ETHREAD
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Ke/sched.h>
#include <Ke/thread.h>

struct ETHREAD {
	KTHREAD thread;
};

OSSTATUS ExCreateThread(ETHREAD **out, void (*entrypoint)(void *), void *arg);

void ExDestroyThread(ETHREAD *thread);

ETHREAD *ExCreateIdleThread(unsigned int cpu);

static inline void ExSetThreadComm(ETHREAD *thread, const char *comm)
{
	KeSetThreadComm(&thread->thread, comm);
}

static inline bool ExWakeThread(ETHREAD *thread)
{
	return KeWakeThread(&thread->thread);
}

