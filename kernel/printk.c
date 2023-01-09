/* SPDX-License-Identifier: MIT */
#include <davix/printk.h>
#include <davix/printk_lib.h>
#include <davix/spinlock.h>

static struct console *atomic first_console = NULL;
static spinlock_t console_lock = SPINLOCK_INIT(console_lock);

void printk_add_console(struct console *console)
{
	console->link = &first_console;
	int irqflag = spin_acquire(&console_lock);
	atomic_store(&console->next, first_console, memory_order_seq_cst);
	if(first_console)
		first_console->link = &console->next;
	atomic_store(&first_console, console, memory_order_seq_cst);
	spin_release(&console_lock, irqflag);
}

static void printk_emit(char loglevel, const char *str)
{
	struct printk_message msg = { .loglevel = loglevel, .string = str };
	for(struct console *console =
		atomic_load(&first_console, memory_order_seq_cst); console;
		console = atomic_load(&console->next, memory_order_seq_cst)) {
			console->emit(console, &msg);
	}
}

void vprintk(char loglevel, const char *fmt, va_list ap)
{
	char buf[1024];
	vsnprintk(buf, 1024, fmt, ap);
	printk_emit(loglevel, buf);
}

void printk(char loglevel, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vprintk(loglevel, fmt, ap);
	va_end(ap);
}

void printk_bust_locks(void)
{
	for(struct console *console = first_console;
		console; console = console->next) {
			if(console->bust_locks)
				console->bust_locks(console);
	}
}
