/**
 * Kernel printk().
 * Copyright (C) 2024  dbstream
 */
#include <davix/console.h>
#include <davix/export.h>
#include <davix/stdarg.h>
#include <davix/stddef.h>
#include <davix/printk.h>
#include <davix/snprintf.h>
#include <davix/time.h>

static void
emit_message (int level, usecs_t msg_time, const char *msg)
{
	/* Clamp the level to between 0 and 5.*/
	if (level < 0 || level > 5)
		level = 1;

	/* Zero means silently ignore. */
	if (!level)
		return;

	arch_printk_emit (level, msg_time, msg);
}

VISIBLE
void
vprintk (const char *fmt, va_list args)
{
	char buf[128];

	usecs_t msg_time = us_since_boot ();

	vsnprintf (buf, sizeof(buf), fmt, args);

	int level = 1;

	const char *s = buf;
	if (s[0] == '\x01' && s[2] == '\x02') {
		level = s[1] - '0';
		s += 3;
	}

	emit_message (level, msg_time, s);
}

VISIBLE
void
printk (const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
	vprintk (fmt, args);
	va_end (args);
}
