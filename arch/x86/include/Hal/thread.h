// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/thread.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <OS/status.h>

struct KTHREAD;

struct HalThreadData {
	void *stack_pointer;
	void *stack_bottom;
};

OSSTATUS HalInitializeThread(
		HalThreadData *thread,
		void (*entrypoint)(void *),
		void *arg
);

void HalDestroyThread(HalThreadData *thread);

void HalInitializeIdleThread(HalThreadData *thread);

static inline KTHREAD *HalCurrentThread(void)
{
	KTHREAD *thread;
	asm volatile("movq %%gs:16, %0" : "=r"(thread));
	return thread;
}

static inline void HalSetCurrentThread(KTHREAD *thread)
{
	asm volatile("movq %0, %%gs:16" :: "r"(thread));
}

