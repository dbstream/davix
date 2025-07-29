// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/davix/vsnprintf.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void vsnprintf [[gnu::format(printf, 3, 0)]] (char *, size_t, const char *, va_list);

void snprintf [[gnu::format(printf, 3, 4)]] (char *, size_t, const char *, ...);

#ifdef __cplusplus
}
#endif

