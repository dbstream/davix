// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/timer.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Ke/time.h>
#include <dsl/avltree.h>

struct KTIMER;

typedef void (*KTIMER_CALLBACK)(KTIMER *);

struct KTIMER {
	dsl::AVLNode node;
	nsec_t expiry;
	KTIMER_CALLBACK callback;
	bool on_queue;

	inline void init(KTIMER_CALLBACK callback_)
	{
		callback = callback_;
		on_queue = false;
	}
};

bool KeSetTimer(KTIMER *timer, nsec_t expiry);

bool KeUnsetTimer(KTIMER *timer);

