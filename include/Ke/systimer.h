// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/systimer.h
 * KSYSTIMER
 *
 * Copyright (C) 2025  dbstream
 *
 * KSYSTIMER implements an API very similar to that of KTIMER, with the major
 * difference that a KSYSTIMER can be dequeued on other CPUs than the enqueueing
 * CPU.
 *
 * KSYSTIMER is implemented in terms of one KTIMER per CPU.
 */
#pragma once

#include <Ke/time.h>
#include <dsl/avltree.h>

struct KSYSTIMER;

typedef void (*KSYSTIMER_CALLBACK)(KSYSTIMER *timer);

struct KSYSTIMER {
	dsl::AVLNode avl;
	nsec_t expiry;
	bool on_queue;
	unsigned int cpu;
	KSYSTIMER_CALLBACK callback;

	inline void init(KSYSTIMER_CALLBACK callback_)
	{
		on_queue = false;
		callback = callback_;
		cpu = 0;
	}
};

bool KeSetSysTimer(KSYSTIMER *timer, nsec_t expiry);

bool KeUnsetSysTimer(KSYSTIMER *timer);

