// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/rcu.h
 * Read-Copy-Update interfaces.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Ke/context.h>

static inline void KeRcuLock(void)
{
	KeDisablePreemption();
}

static inline void KeRcuUnlock(void)
{
	KeEnablePreemption();
}

