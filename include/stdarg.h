// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/stdarg.h
 *
 * Copyright (C) 2025  dbstream
 */
#ifndef __DAVIX_KERNEL__
#include_next <stdarg.h>
#else

typedef __builtin_va_list va_list;

#define va_start(va, arg) __builtin_va_start(va, arg)
#define va_end(va) __builtin_va_end(va)
#define va_arg(va, typ) __builtin_va_arg(va, typ)

#endif /* __DAVIX_KERNEL__ */

