/**
 * Init executable.
 * Copyright (C) 2025-present  dbstream
 *
 * This is passed as the boot module to the kernel.
 */
#include <asm/syscall_nr.h>

/**
 * memcpy, memset, and memmove are implemented by the arch, not util/string.c.
 * This means we need to implement them here, as we cannot depend on string.c to
 * provide them.  :-(
 */

#undef memcpy
#undef memset
#undef memmove

void *
memcpy (void *dst, const void *src, unsigned long n)
{
	char *d = dst;
	const char *s = src;
	for (; n; n--)
		*(d++) = *(s++);
	return dst;
}

void *
memset (void *dst, int x, unsigned long n)
{
	char *d = (char *) dst;
	for (; n; n--)
		*(d++) = x;
	return dst;
}

void *
memmove (void *dst, const void *src, unsigned long n)
{
	if ((unsigned long) dst <= (unsigned long) src)
		return memcpy (dst, src, n);

	char *d = (char *) dst;
	char *s = (char *) src;
	while (n--)
		d[n] = s[n];
	return dst;
}

#include "util/snprintf.c"
#include "util/string.c"

static void
dmesg (const char *fmt, ...)
{
	char buf[128];

	va_list ap;
	va_start (ap, fmt);
	vsnprintf (buf, sizeof (buf), fmt, ap);
	va_end (ap);

	asm volatile ("syscall" :: "a" (__SYS_write_dmesg),
			"D" (buf),
			"S" (strnlen (buf, sizeof (buf)))
			: "rdx", "r10", "r8", "r9", "cc", "rcx", "r11");
}

void
_start (void)
{
	/** note that we're alive...  */
	dmesg ("\"%s\" from userspace!\n", "hello world");

	/** ... and now sleep... */
	asm volatile ("syscall" :: "a" (__SYS_exit),
			"D" (0)
			: "rsi", "rdx", "r10", "r8", "r9", "cc", "rcx", "r11");
}
