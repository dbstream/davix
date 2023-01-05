/* SPDX-License-Identifier: MIT */
#include <asm/boot.h>
#include <asm/io.h>
#include "printk.h"
#include "../../../kernel/printk_lib.c"

static const char LOGLEVEL =
#ifdef CONFIG_LOADER_LOGLEVEL_DEBUG
	KERN_DEBUG
#endif
#ifdef CONFIG_LOADER_LOGLEVEL_INFO
	KERN_INFO
#endif
#ifdef CONFIG_LOADER_LOGLEVEL_WARN
	KERN_WARN
#endif
#ifdef CONFIG_LOADER_LOGLEVEL_ERROR
	KERN_ERROR
#endif
;

#define COM1 0x3f8

static int has_uart = 0;

static void uart_write(u16 iobase, u8 value)
{
	while(!(io_inb(iobase + 5) & 0x20))
		__builtin_ia32_pause();
	io_outb(iobase, value);
}

static void uart_write_printk(u16 iobase, char loglevel, const char *s)
{
	if(loglevel)
		uart_write(iobase, loglevel);
	for(; *s; s++) {
		if(*s == '\n')
			uart_write(iobase, '\r');
		uart_write(iobase, *s);
	}
}

void printk(char loglevel, const char *fmt, ...)
{
	if(LOGLEVEL > loglevel)
		return;

	if(!has_uart)
		return;

	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintk(buf, 1024, fmt, ap);
	va_end(ap);
	uart_write_printk(COM1, loglevel, buf);
}

void uart_init(void)
{
	unsigned int baudrate = 38400; /* change this to whatever */
	u16 divisor = 115200 / baudrate;
	io_outb(COM1 + 1, 0);		/* disable the UART */
	io_outb(COM1 + 3, 1 << 7);	/* set baud rate divisor */
	io_outb(COM1 + 0, divisor);		/* clock rate divisor (low byte) */
	io_outb(COM1 + 1, divisor >> 8);	/* clock rate divisor (high byte) */
	io_outb(COM1 + 3, 3);		/* 8 bits, no parity, one stop bit */
	io_outb(COM1 + 2, 0xc7);
	io_outb(COM1 + 4, 0xb);		/* enable the UART */

	io_outb(COM1 + 4, 0x1e);	/* set loopback mode */
	io_outb(COM1, 0xdb);
	if(io_inb(COM1) != 0xdb)	/* make sure that the UART is not faulty */
		return;

	io_outb(COM1 + 4, 0x0f);	/* resume normal operation */
	has_uart = 1;

	for(const char *s = "\ec\e[0m\e[?25h\e[H\e[J\e[3J\n";
		*s; s++)
		uart_write(COM1, *s);
}

void uart_fillin_boot_struct(struct boot_struct *boot_struct)
{
	boot_struct->com1_initialized = has_uart ? 1 : 0;
	boot_struct->com2_initialized = 0;
}
