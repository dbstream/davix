/**
 * stddef.h freestanding header
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

typedef __PTRDIFF_TYPE__ ssize_t;

static_assert (sizeof (size_t) == sizeof (ssize_t));

#ifdef __cplusplus
	typedef decltype(nullptr) nullptr_t;
#else
	typedef typeof(nullptr) nullptr_t;
#endif

#ifdef __cplusplus
#	define NULL 0
#else
#	define NULL ((void *) 0)
#endif

#define offsetof(t, m) __builtin_offsetof(t, m)

#define unreachable() __builtin_unreachable()
