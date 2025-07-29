// SPDX-License-Identifier: GPL-3.0
/*
 * File: lib/string.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <davix/export.h>
#include <string.h>

#undef memset
#undef mempcpy
#undef memcpy
#undef memmove
#undef memcmp
#undef strlen
#undef stpcpy
#undef strcpy
#undef strcmp
#undef bzero

extern "C"
void *memset(void *dst, int c, size_t n)
{
	char *p = (char *) dst;
	for (; n; p++, n--)
		*p = c;
	return dst;
}
EXPORT_SYMBOL(memset)

extern "C"
void *mempcpy(void *__restrict__ dst, const void *__restrict__ src, size_t n)
{
	char *d = (char *) dst;
	char *s = (char *) src;
	for (; n; d++, s++, n--)
		*d = *s;
	return (void *) d;
}
EXPORT_SYMBOL(mempcpy)

extern "C"
void *memcpy(void *__restrict__ dst, const void *__restrict__ src, size_t n)
{
	mempcpy(dst, src, n);
	return dst;
}
EXPORT_SYMBOL(memcpy)

extern "C"
void *memmove(void *dst, const void *src, size_t n)
{
	char *d = (char *) dst;
	char *s = (char *) src;

	if (d > s) {
		while (n--)
			d[n] = s[n];
	} else {
		for (; n; d++, s++, n--)
			*d = *s;
	}

	return dst;
}
EXPORT_SYMBOL(memmove)

extern "C"
int memcmp(const void *s1, const void *s2, size_t n)
{
	unsigned char *a = (unsigned char *) s1;
	unsigned char *b = (unsigned char *) s2;

	for (; n; a++, b++, n--) {
		if (*a != *b)
			return *a - *b;
	}
	return 0;
}
EXPORT_SYMBOL(memcmp)

extern "C"
size_t strlen(const char *s)
{
	size_t i = 0;
	for (; s[i]; i++)
		;
	return i;
}
EXPORT_SYMBOL(strlen)

extern "C"
char *stpcpy(char *__restrict__ dst, const char *__restrict__ src)
{
	for (; *src; dst++, src++)
		*dst = *src;
	*dst = 0;
	return dst;
}
EXPORT_SYMBOL(stpcpy)

extern "C"
char *strcpy(char *__restrict__ dst, const char *__restrict__ src)
{
	stpcpy(dst, src);
	return dst;
}
EXPORT_SYMBOL(strcpy)

extern "C"
int strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2) {
		if (!*s2)
			return 0;
		s1++;
		s2++;
	}
	return *(unsigned char *) s1 - *(unsigned char *) s2;
}
EXPORT_SYMBOL(strcmp)

extern "C"
size_t strnlen(const char *s, size_t n)
{
	size_t i = 0;
	for (; i < n && s[i]; i++)
		;
	return i;
}
EXPORT_SYMBOL(strnlen)

extern "C"
char *stpncpy (char *__restrict__ dst, const char *__restrict__ src, size_t n)
{
	for (; n && *src; dst++, src++, n--)
		*dst = *src;
	char *ret = dst;
	for (; n; dst++, n--)
		*dst = 0;
	return ret;
}
EXPORT_SYMBOL(stpncpy)

extern "C"
char *strncpy(char *__restrict__ dst, const char *__restrict__ src, size_t n)
{
	stpncpy(dst, src, n);
	return dst;
}
EXPORT_SYMBOL(strncpy)

extern "C"
int strncmp(const char *s1, const char *s2, size_t n)
{
	for (; n; s1++, s2++, n--) {
		if (*s1 != *s2)
			return *(const unsigned char *) s1 - *(const unsigned char *) s2;
		if (!*s1)
			return 0;
	}
	return 0;
}
EXPORT_SYMBOL(strncmp)

extern "C"
const char *strchrnul(const char *s, char c)
{
	for (; *s != c; s++) {
		if (!*s)
			break;
	}
	return s;
}
EXPORT_SYMBOL(strchrnul)

extern "C"
void bzero(void *mem, size_t n)
{
	memset(mem, 0, n);
}
EXPORT_SYMBOL(bzero)

extern "C"
void bzero_explicit(void *mem, size_t n)
{
	memset(mem, 0, n);
	asm volatile("" :: "m"(mem) : "memory");
}
EXPORT_SYMBOL(bzero_explicit)

