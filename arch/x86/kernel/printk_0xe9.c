/**
 * arch_printk_emit() to port 0xe9
 * Copyright (C) 2024  dbstream
 *
 * QEMU and Bochs implement a "debug console" that runs on port 0xe9, which is
 * an easy form of writing output for kernel debugging purposes.
 */
#include <davix/console.h>
#include <davix/context.h>
#include <davix/snprintf.h>
#include <davix/spinlock.h>
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

/** HACK to avoid messy output on 0xe9 when there are multiple CPUs  */
static spinlock_t lock;

/**
 * status:
 *   bit 0: irq flag
 *   bit 1: ungrab lock
 */
static int
grab_lock (void)
{
	/** NB: we cannot spin on the lock when we are in NMI  */
	int status = irq_save () ? 1 : 0;
	if (in_nmi ())
		status |= spin_trylock (&lock) ? 2 : 0;
	else {
		status |= 2;
		__spin_lock (&lock);
	}

	return status;
}

static void
ungrab_lock (int status)
{
	if (status & 2)
		__spin_unlock (&lock);

	irq_restore ((status & 1) ? true : false);
}

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

	int status = grab_lock ();
	write_string ("\e[32m");
	write_string (buf);
	if (in_nmi ())	write_string ("\e[1mNMI ");
	if (in_irq ())	write_string ("\e[1mIRQ ");
	write_string (msg_prefix[level]);
	write_string (msg);
	write_string ("\e[0m");
	ungrab_lock (status);
}
