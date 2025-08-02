// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/dpc.h
 * Deferred procedure calls.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <dsl/list.h>

struct DPC;

typedef void (*DPC_CALLBACK)(DPC *dpc, void *context, void *arg1, void *arg2);

struct DPC {
	dsl::ListHead list_entry;
	DPC_CALLBACK callback;
	void *context;
	bool on_queue;

	void *arg1;
	void *arg2;

	inline void init(DPC_CALLBACK callback_, void *arg1_, void *arg2_)
	{
		on_queue = false;
		callback = callback_;
		arg1 = arg1_;
		arg2 = arg2_;
	}
};

bool KeEnqueueDPC(DPC *dpc, void *context);

