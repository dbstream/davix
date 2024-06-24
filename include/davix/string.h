/**
 * String functions.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_STRING_H
#define _DAVIX_STRING_H 1

#define memset	__builtin_memset
#define memcpy	__builtin_memcpy
#define memmove	__builtin_memmove
#define memcmp	__builtin_memcmp

#define strlen __builtin_strlen
#define strcpy __builtin_strcpy
#define stpcpy __builtin_stpcpy
#define strcmp __builtin_strcmp

extern unsigned long
strnlen (const char *s, unsigned long n);

extern char *
strncpy (char *dst, const char *src, unsigned long n);

extern char *
strpcpy (char *dst, const char *src, unsigned long n);

extern int
strncmp (const char *s1, const char *s2, unsigned long n);

#endif /* _DAVIX_STRING_H */
