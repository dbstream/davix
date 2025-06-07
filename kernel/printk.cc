/**
 * Kernel printk().
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/atomic.h>
#include <davix/console.h>
#include <davix/printk.h>
#include <davix/spinlock.h>
#include <davix/time.h>
#include <vsnprintf.h>

static Console *console_list;
static spinlock_t console_lock;

/**
 * Register a new console as a printk output.
 * @con: console to register
 */
void
console_register (Console *con)
{
	scoped_spinlock_dpc g (console_lock);

	con->link = &console_list;
	con->next = console_list;
	if (console_list)
		console_list->link = &con->next;
	atomic_store (&console_list, con, mo_seq_cst);
}

static spinlock_t printk_output_lock;

/**
 * Emit a message with printk.
 * @level: log level to print at
 * @msg_time: message time
 * @msg: the formatted message to print
 */
static void
printk_emit (int level, usecs_t msg_time, const char *msg)
{
	printk_output_lock.lock_irq ();

	Console *con = atomic_load (&console_list, mo_seq_cst);
	for (; con; con = con->next) {
		con->emit_message (con, level, msg_time, msg);
	}

	printk_output_lock.unlock_irq ();
}

extern "C"
void
printk (const char *fmt, ...)
{
	char buf[768];

	usecs_t msg_time = us_since_boot ();

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

	printk_emit (level, msg_time, p);
}
