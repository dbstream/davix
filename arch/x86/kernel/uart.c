/* SPDX-License-Identifier: MIT */
#include <davix/printk.h>
#include <davix/spinlock.h>
#include <asm/boot.h>
#include <asm/io.h>
#include "uart.h"

/*
 * A kernel driver implementing early printk() to COM1 and COM2.
 */

struct uart_chip {
	struct console console;
	spinlock_t lock;
	u16 io_base;
};

static void uart_put_raw(struct uart_chip *uart, char c)
	must_hold(&uart->lock)
{
	while(!(io_inb(uart->io_base + 5) & 0x20))
		relax();
	io_outb(uart->io_base, c);
} 

static void uart_put(struct uart_chip *uart, char c)
	must_hold(&uart->lock)
{
	if(c == '\n')
		uart_put_raw(uart, '\r');
	uart_put_raw(uart, c);
}

static void uart_emit(struct console *console,
	const struct printk_message *msg)
{
	struct uart_chip *uart =
		container_of(console, struct uart_chip, console);
	int irqflag = spin_acquire(&uart->lock);
	if(msg->loglevel)
		uart_put(uart, msg->loglevel);
	for(const char *s = msg->string; *s; s++)
		uart_put(uart, *s);
	spin_release(&uart->lock, irqflag);
}

static int uart_probe(struct uart_chip *uart, unsigned baudrate)
{
	u16 baudrate_div = 115200 / baudrate;

	io_outb(uart->io_base + 1, 0);
	io_outb(uart->io_base + 3, 1 << 7);
	io_outb(uart->io_base + 0, baudrate_div);
	io_outb(uart->io_base + 1, baudrate_div >> 8);
	io_outb(uart->io_base + 3, 3);
	io_outb(uart->io_base + 2, 0xc7);
	io_outb(uart->io_base + 4, 0xb);

	io_outb(uart->io_base + 4, 0x1e);
	io_outb(uart->io_base, 0xdb);
	if(io_inb(uart->io_base) != 0xdb)
		return 0;

	io_outb(uart->io_base + 4, 0x0f);
	return 1;
}

static struct uart_chip COM1 = {
	.console.emit = uart_emit,
	.lock = SPINLOCK_INIT(COM1.lock),
	.io_base = 0x3f8
};

static struct uart_chip COM2 ={
	.console.emit = uart_emit,
	.lock = SPINLOCK_INIT(COM1.lock),
	.io_base = 0x2f8
};

void x86_uart_init(void)
{
	if(x86_boot_struct.com1_initialized || uart_probe(&COM1, 38400)) {
		printk_add_console(&COM1.console);
		debug("console: added COM1\n");
	}

	if(x86_boot_struct.com2_initialized || uart_probe(&COM2, 38400)) {
		printk_add_console(&COM2.console);
		debug("console: added COM2\n");
	}
}
