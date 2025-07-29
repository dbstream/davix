// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/davix/bug.h
 * BUG_* macros.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Ke/log.h>

#define BUG() do {							\
	[[unlikely]];							\
	KePanic("BUG! in %s() (%s:%d)", __func__, __FILE__, __LINE__);	\
} while (0)

#define BUG_ON(expr) do { if (expr) BUG(); } while (0)

#define WARN() do {							\
	[[unlikely]];							\
	KePrintf("WARNING! in %s() (%s:%d)\n", __func__, __FILE__, __LINE__); \
} while (0)

#define WARN_ON(expr) ({ bool b = !!(expr); if (b) WARN(); b; })

