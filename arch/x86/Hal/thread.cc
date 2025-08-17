// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/thread.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ke/context.h>
#include <Ke/log.h>
#include <Ke/thread.h>
#include <Ki/sched.h>
#include <Mm/vmap.h>
#include <asm/entry.h>
#include <string.h>

struct task_switch_frame {
	unsigned long rbp;
	unsigned long rbx;
	unsigned long r12;
	unsigned long r13;
	unsigned long r14;
	unsigned long r15;
	unsigned long ip;
	unsigned long padding;
	entry_regs initial_entry_regs;
};

static_assert(sizeof(task_switch_frame) == 0xe8);

extern "C" char asm_ret_from_new_thread[];

extern "C" KTHREAD *asm_switch_thread(KTHREAD *previous, KTHREAD *next);

KTHREAD *HalSwitchThread(KTHREAD *previous, KTHREAD *next)
{
	HalSetCurrentThread(next);

	return asm_switch_thread(previous, next);
}

extern "C" void HalReturnFromNewThread(
	KTHREAD *previous,
	void (*entry_function)(void *),
	void *arg
)
{
	KiSetupInitialThreadContext();
	KiFinalizeTaskSwitch(previous);
	KeEnablePreemption();

	entry_function(arg);

	KePanic("HalReturnFromNewThread: entry_function returned!");
}

OSSTATUS HalInitializeThread(
	HalThreadData *thread,
	void (*entrypoint)(void *),
	void *arg
)
{
	thread->stack_bottom = MmAllocateVirtual(0x4000, PTEFLAGS_READWRITE);
	if (!thread->stack_bottom)
		return OS_STATUS_KERNEL_ALLOCATION_FAILED;

	task_switch_frame *initial_frame = (task_switch_frame *)
		((unsigned long) thread->stack_bottom + 0x4000 - sizeof(*initial_frame));
	thread->stack_pointer = initial_frame;

	bzero(initial_frame, sizeof(*initial_frame));
	initial_frame->ip = (unsigned long) asm_ret_from_new_thread;
	initial_frame->r12 = (unsigned long) entrypoint;
	initial_frame->r13 = (unsigned long) arg;

	return OS_STATUS_SUCCESS;
}

void HalDestroyThread(HalThreadData *thread)
{
	MmFreeVirtual(thread->stack_bottom);
}

void HalInitializeIdleThread(HalThreadData *thread)
{
	thread->stack_bottom = nullptr;
	thread->stack_pointer = nullptr;
}

