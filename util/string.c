/**
 * Implementations for most of the <string.h> functions.
 * Copyright (C) 2024  dbstream
 *
 * NOTE: the following are implemented by architectures:
 *	- memcpy
 *	- memset
 *	- memmove
 */
#include <davix/string.h>
#include <davix/string.h>

#undef strlen
#undef stpcpy
#undef strcpy
#undef strcmp
#undef memcmp

unsigned long
strlen(const char *s)
{
        unsigned long i = 0;
        while(s[i])
                i++;
        return i;
}

unsigned long
strnlen(const char *s, unsigned long n)
{
        unsigned long i = 0;
        while(i < n) {
                if(!s[i])
                        break;
                i++;
        }
        return i;
}

char *
stpcpy(char *restrict dst, const char *restrict src)
{
        for(; *src; dst++, src++)
                *dst = *src;
        *dst = 0;
        return dst;
}

char *
strcpy(char *restrict dst, const char *restrict src)
{
        stpcpy(dst, src);
        return dst;
}

char *
stpncpy(char *restrict dst, const char *restrict src, unsigned long n)
{
        unsigned long len = strnlen(src, n);
        memcpy(dst, src, len);
        memset(dst + len, 0, n - len);
        return dst + len;
}

char *
strncpy(char *restrict dst, const char *restrict src, unsigned long n)
{
        stpncpy(dst, src, n);
        return dst;
}

int
memcmp(const void *s1, const void *s2, unsigned long n)
{
        unsigned char *a = (unsigned char *) s1;
        unsigned char *b = (unsigned char *) s2;

        for(; n; a++, b++, n--)
                if(*a != *b)
                        return *a - *b;
        return 0;
}

int
strcmp(const char *s1, const char *s2)
{
        while(*s1 == *s2) {
                if(!*s1)
                        return 0;
                s1++;
                s2++;
        }

        return *(unsigned char *) s1 - *(unsigned char *) s2;
}

int
strncmp(const char *s1, const char *s2, unsigned long n)
{
        while(n) {
                if(*s1 != *s2)
                        return *(unsigned char *) s1 - *(unsigned char *) s2;
                if(!*s1)
                        return 0;
                s1++;
                s2++;
                n--;
        }

        return 0;
}

const char *
strchrnul (const char *s, char c)
{
	while (*s != c) {
		if (!*s)
			break;
		s++;
	}
	return s;
}
