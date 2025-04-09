/**
 * Functions one would expect to find in string.h
 * Copyright (C) 2025-present  dbstream
 */
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

#ifdef __cplusplus
extern "C"
#endif
size_t
strnlen (const char *s, size_t n);

#ifdef __cplusplus
extern "C"
#endif
char *
strncpy (char *__restrict__ dst, const char *__restrict__ src, size_t n);

#ifdef __cplusplus
extern "C"
#endif
char *
stpncpy (char *__restrict__ dst, const char *__restrict__ src, size_t n);

#ifdef __cplusplus
extern "C"
#endif
int
strncmp (const char *s1, const char *s2, size_t n);

#ifdef __cplusplus
extern "C"
#endif
const char *
strchrnul (const char *s, char c);
