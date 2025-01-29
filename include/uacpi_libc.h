/**
 * uACPI libc definitions for Davix.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX__UACPI_LIBC_H
#define _DAVIX__UACPI_LIBC_H 1

#include <davix/snprintf.h>
#include <davix/string.h>

#define uacpi_memcpy memcpy
#define uacpi_memset memset
#define uacpi_memcmp memcmp
#define uacpi_strcmp strcmp
#define uacpi_memmove memmove
#define uacpi_strnlen strnlen
#define uacpi_strlen strlen
#define uacpi_snprintf snprintf
#define uacpi_vsnprintf vsnprintf

#endif /* _DAVIX__UACPI_LIBC_H */

