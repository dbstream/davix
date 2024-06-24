/**
 * C stddef header.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_STDDEF_H
#define _DAVIX_STDDEF_H 1

#define NULL 0

typedef long ssize_t;
typedef unsigned long size_t;

#define offsetof(t, n) __builtin_offsetof(t, n)

#define min(t, a, b) ({		\
	t _a = (a);		\
	t _b = (b);		\
	_a < _b ? _a : _b; })
#define max(t, a, b) ({		\
	t _a = (a);		\
	t _b = (b);		\
	_a > _b ? _a : _b; })

#endif /* _DAVIX_STDDEF_H */
