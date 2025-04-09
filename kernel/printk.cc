/**
 * Kernel printk().
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/printk.h>
#include <vsnprintf.h>

/**
 * HACK: printk to 0xe9 for now, with no locking.
 */
#include <asm/io.h>

static const char *prefix[5] = {
	"",
	"\x1b[0m",
	"\x1b[0;1m",
	"\x1b[0;33m",
	"\x1b[0;31m"
};

static void
emitmsg (int level, const char *msg)
{
	for (const char *s = prefix[level]; *s; s++)
		io_outb (0xe9, *s);
	for (; *msg; msg++)
		io_outb (0xe9, *msg);
}

extern "C"
void
printk (const char *fmt, ...)
{
	char buf[512];

	va_list args;
	va_start (args, fmt);
	vsnprintf (buf, sizeof (buf), fmt, args);
	va_end (args);

	int level = 0;
	const char *p = buf;
	if (p[0] == 0x01 && p[1] && p[2] == 0x02) {
		level = p[1] - '0';
		p += 3;
	}

	if (level < 0 || level > 4)
		level = 0;

	emitmsg (level, p);
}
