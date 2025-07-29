// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/stddef.h
 *
 * Copyright (C) 2025  dbstream
 */
#ifndef __DAVIX_KERNEL__
#include_next <stddef.h>
#else
#pragma once
typedef unsigned long size_t;
typedef long ptrdiff_t;

typedef long ssize_t;

#ifdef __cplusplus
#ifdef __SIZE_TYPE__
static_assert(sizeof(long) == sizeof(__SIZE_TYPE__));
#endif
#ifdef __PTRDIFF_TYPE
static_assert(sizeof(long) == sizeof(__PTRDIFF_TYPE__));
#endif
#endif

#ifdef __cplusplus
typedef decltype(nullptr) nullptr_t;
#else
typedef typeof(nullptr) nullptr_t;
#endif

#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *) 0)
#endif

#define offsetof(t, m) __builtin_offsetof(t, m)

#define unreachable() __builtin_unreachable()

#endif /* __DAVIX_KERNEL__ */

