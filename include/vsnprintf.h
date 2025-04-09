/**
 * vsnprintf.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdarg.h>
#include <stddef.h>

/**
 * We support the following subset of printf() functionality:
 *
 * - %% to escape a percentage sign.
 *
 * - hh, h, l, ll, t, z, j size modifiers.
 *
 * - %c, %s, %d, %i, %u, %o, %x, %X, %p format specifiers.
 *
 * - '-', ' ', '+', '0', '#' modifiers.  If both '0' and '-' are specified,
 *   results may be inconsistent.
 *
 * - field width: supported for %s, %d, %i, %u, %o, %x, %X.  field width may be
 *
 * - precision: supported for %s.  ignored by others (nonconformant).
 *
 * - field width and precision may be specified as immediates (%12.34s) or as
 *   indirect arguments (%*.*s)
 *
 * We do not support the following features: %n, %<idx>$..., and floating-point
 * format specifiers.  We handle format errors by writing "(bad format string)"
 * to the output buffer (probably nonconformant).
 */

#ifdef __cplusplus
extern "C"
#endif
void
__attribute__ ((format (printf, 3, 0)))
vsnprintf (char *buf, size_t size, const char *fmt, va_list args);

#ifdef __cplusplus
extern "C"
#endif
void
__attribute__ ((format (printf, 3, 4)))
snprintf (char *buf, size_t size, const char *fmt, ...);
