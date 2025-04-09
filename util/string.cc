/**
 * Functions one would expect to find in string.h
 * Copyright (C) 2025-present  dbstream
 */
#include <string.h>

#undef memset

extern "C" void *
memset (void *dst, int c, size_t n)
{
	unsigned char *p = (unsigned char *) dst;
	for (; n; p++, n--)
		*p = c;
	return dst;
}

#undef mempcpy
extern "C" void *
mempcpy (void *__restrict__ dst, const void *__restrict__ src, size_t n)
{
	unsigned char *__restrict__ d = (unsigned char *__restrict__) dst;
	unsigned char *__restrict__ s = (unsigned char *__restrict__) src;
	for (; n; d++, s++, n--)
		*d = *s;
	return (void *) d;
}

#undef memcpy
extern "C" void *
memcpy (void *__restrict__ dst, const void *__restrict__ src, size_t n)
{
	mempcpy (dst, src, n);
	return dst;
}

#undef memmove
extern "C" void *
memmove (void *dst, const void *src, size_t n)
{
	unsigned char *d = (unsigned char *) dst;
	unsigned char *s = (unsigned char *) src;

	if (d > s) {
		while (n--)
			d[n] = s[n];
	} else {
		for (; n; d++, s++, n--)
			*d = *s;
	}

	return dst;
}

#undef memcmp
extern "C" int
memcmp (const void *s1, const void *s2, size_t n)
{
	unsigned char *a = (unsigned char *) s1;
	unsigned char *b = (unsigned char *) s2;

	for (; n; a++, b++, n--) {
		if (*a != *b)
			return *a - *b;
	}
	return 0;
}

#undef strlen
extern "C" size_t
strlen (const char *s)
{
	size_t i = 0;
	for (; s[i]; i++);
	return i;
}

#undef stpcpy
extern "C" char *
stpcpy (char *__restrict dst, const char *__restrict__ src)
{
	for (; *src; dst++, src++)
		*dst = *src;
	*dst = 0;
	return dst;
}

#undef strcpy
extern "C" char *
strcpy (char *__restrict__ dst, const char *__restrict__ src)
{
	stpcpy (dst, src);
	return dst;
}

#undef strcmp
extern "C" int
strcmp (const char *s1, const char *s2)
{
	while (*s1 == *s2) {
		if (!*s1)
			return 0;
		s1++;
		s2++;
	}
	return *(unsigned char *) s1 - *(unsigned char *) s2;
}

extern "C" size_t
strnlen (const char *s, size_t n)
{
	size_t i = 0;
	for (; i < n && s[i]; i++);
	return i;
}

extern "C" char *
strncpy (char *__restrict__ dst, const char *__restrict__ src, size_t n)
{
	stpncpy (dst, src, n);
	return dst;
}

extern "C" char *
stpncpy (char *__restrict__ dst, const char *__restrict__ src, size_t n)
{
	for (; n && *src; dst++, src++, n--)
		*dst = *src;
	char *ret = dst;
	for (; n; dst++, n--)
		*dst = 0;
	return ret;
}

extern "C" int
strncmp (const char *s1, const char *s2, size_t n)
{
	for (; n; s1++, s2++, n--) {
		if (*s1 != *s2)
			return *(unsigned char *) s1 - *(unsigned char *) s2;
		if (!*s1)
			return 0;
	}
	return 0;
}

extern "C" const char *
strchrnul (const char *s, char c)
{
	for (; *s != c; s++) {
		if (!*s)
			break;
	}
	return s;
}
