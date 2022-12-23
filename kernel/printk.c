/* SPDX-License-Identifier: MIT */
#include <davix/printk.h>
#include <davix/printk_lib.h>
#include <davix/spinlock.h>

static struct console *atomic first_console = NULL;
static spinlock_t console_lock = SPINLOCK_INIT(console_lock);

void printk_add_console(struct console *console)
{
	console->link = &first_console;
	spin_acquire(&console_lock);
	atomic_store(&console->next, first_console, memory_order_seq_cst);
	if(first_console)
		first_console->link = &console->next;
	atomic_store(&first_console, console, memory_order_seq_cst);
	spin_release(&console_lock);
}

void printk(char loglevel, const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintk(buf, 1024, fmt, ap);

	struct printk_message msg = { .loglevel = loglevel, .string = buf };

	for(struct console *console = atomic_load(&first_console, memory_order_seq_cst);
		console; console = atomic_load(&console->next, memory_order_seq_cst)) {
			console->emit(console, &msg);
	}
}
