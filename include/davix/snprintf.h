/**
 * (v)snprintf
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_SNPRINTF_H
#define _DAVIX_SNPRINTF_H 1

#include <davix/stdarg.h>

__attribute__ ((format (printf, 3, 0)))
extern int
vsnprintf (char *buf, unsigned long size, const char *fmt, va_list args);

__attribute__ ((format (printf, 3, 4)))
extern int
snprintf (char *buf, unsigned long size, const char *fmt, ...);

#endif /* _DAVIX_SNPRINTF_H */
