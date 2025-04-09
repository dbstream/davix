/**
 * stdarg.h freestanding header
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

typedef __builtin_va_list va_list;

#define va_start(v, arg) __builtin_va_start(v, arg)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, tp) __builtin_va_arg(v, tp)
