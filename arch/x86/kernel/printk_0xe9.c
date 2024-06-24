/**
 * arch_printk_emit() to port 0xe9
 * Copyright (C) 2024  dbstream
 *
 * QEMU and Bochs implement a "debug console" that runs on port 0xe9, which is
 * an easy form of writing output for kernel debugging purposes.
 */
#include <davix/console.h>
#include <davix/snprintf.h>
#include <davix/stddef.h>
#include <asm/io.h>
#include <asm/irq.h>

static void
write_string (const char *s)
{
	for (; *s; s++)
		io_outb (0xe9, *s);
}

const char *msg_prefix[] = {
	NULL,
	"\e[0m",
	"\e[0;1m",
	"\e[1;33m",
	"\e[31m",
	"\e[1;31m"
};

void
arch_printk_emit (int level, usecs_t msg_time, const char *msg)
{
	if (level < 1)
		level = 1;
	else if (level > 5)
		level = 5;

	char buf[24];
	snprintf (buf, sizeof (buf), "[%5llu.%06llu] ",
		msg_time / 1000000, msg_time % 1000000);

	bool flag = irq_save ();
	write_string ("\e[32m");
	write_string (buf);
	write_string (msg_prefix[level]);
	write_string (msg);
	write_string ("\e[0m");
	irq_restore (flag);
}
