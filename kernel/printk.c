/**
 * Kernel printk().
 * Copyright (C) 2024  dbstream
 */
#include <davix/console.h>
#include <davix/export.h>
#include <davix/spinlock.h>
#include <davix/stdarg.h>
#include <davix/stddef.h>
#include <davix/printk.h>
#include <davix/snprintf.h>
#include <davix/syscall.h>
#include <davix/time.h>
#include <asm/usercopy.h>

static spinlock_t console_lock;
static DEFINE_EMPTY_LIST (console_list);

void
console_register (struct console *con)
{
	bool flag = spin_lock_irq (&console_lock);
	list_insert (&console_list, &con->list);
	spin_unlock_irq (&console_lock, flag);
}

void
console_unregister (struct console *con)
{
	bool flag = spin_lock_irq (&console_lock);
	list_delete (&con->list);
	spin_unlock_irq (&console_lock, flag);
}

static void
__emit_message (int level, usecs_t msg_time, const char *msg)
{
	list_for_each (entry, &console_list) {
		struct console *console = list_item (struct console, list, entry);
		console->emit_message (console, level, msg_time, msg);
	}
}

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

	if (in_nmi ()) {
		if (spin_trylock (&console_lock)) {
			__emit_message (level, msg_time, msg);
			__spin_unlock (&console_lock);
		}
	} else {
		bool flag = spin_lock_irq (&console_lock);
		__emit_message (level, msg_time, msg);
		spin_unlock_irq (&console_lock, flag);
	}
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

SYSCALL2 (void, write_dmesg, const void *, msg, size_t, msg_len)
{
	if (msg_len >= 128)
		syscall_return_error (E2BIG);

	usecs_t msg_time = us_since_boot ();

	char buf[128];
	errno_t e = memcpy_from_userspace (buf, msg, msg_len);
	if (e != ESUCCESS)
		syscall_return_error (EFAULT);

	buf[msg_len] = 0;
	emit_message (1, msg_time, buf);
	syscall_return_void;
}
