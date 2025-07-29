// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/string.h
 *
 * Copyright (C) 2025  dbstream
 */
#ifndef __DAVIX_KERNEL__
#include_next <string.h>
#else
#pragma once

#include <stddef.h>

#define memset __builtin_memset
#define memcpy __builtin_memcpy
#define mempcpy __builtin_mempcpy
#define memmove __builtin_memmove
#define memcmp __builtin_memcmp
#define strlen __builtin_strlen
#define strcpy __builtin_strcpy
#define stpcpy __builtin_stpcpy
#define strcmp __builtin_strcmp
#define bzero __builtin_bzero

#ifdef __cplusplus
extern "C" {
#endif

size_t strnlen(const char *, size_t);
char *stpncpy(char *__restrict__, const char *__restrict__, size_t);
char *strncpy(char *__restrict__, const char *__restrict__, size_t);
int strncmp(const char *, const char *, size_t);
const char *strchrnul(const char *, char);
void bzero_explicit(void *, size_t);

#ifdef __cplusplus
}
#endif

#endif /* __DAVIX_KERNEL__ */

