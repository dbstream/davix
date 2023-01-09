/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_PRINTK_H
#define __DAVIX_PRINTK_H

#include <davix/atomic.h>
#include <davix/types.h>

struct printk_message {
	char loglevel;
	const char *string;
};

/*
 * printk() console interface. This is usually embedded in other structures,
 * such as ``struct uart_chip`` for UARTs.
 */
struct console {
	/*
	 * Write a log message.
	 */
	void (*emit)(struct console *console, const struct printk_message *msg);

	/*
	 * Reset all spinlocks used by the console, for panic() scenarios.
	 */
	void (*bust_locks)(struct console *console);

	/*
	 * Linked list of consoles.
	 *
	 * The rules are:
	 *
	 * -) ``console->next`` must always be valid, that is, either NULL or
	 *    pointing to a valid console.
	 *
	 * -) When panic() is called, an operation involving an update of the
	 *    console list may be stopped when it's half-way done, and that
	 *    should not be a problem.
	 */
	struct console *atomic next;	/* Pointer to the next, or NULL. */
	struct console *atomic *link;	/* Pointer to the link to this. Remember
				 * to update next->link. */
};

void printk_add_console(struct console *console);

void vprintk(char loglevel, const char *fmt, va_list ap);
void printk(char loglevel, const char *fmt, ...);

#define KERN_DEBUG '0'
#define KERN_INFO '1'
#define KERN_WARN '2'
#define KERN_ERROR '3'
#define KERN_CRITICAL '4'

#define debug(...) printk(KERN_DEBUG, __VA_ARGS__)
#define info(...) printk(KERN_INFO, __VA_ARGS__)
#define warn(...) printk(KERN_WARN, __VA_ARGS__)
#define error(...) printk(KERN_ERROR, __VA_ARGS__)
#define critical(...) printk(KERN_CRITICAL, __VA_ARGS__)

void printk_bust_locks(void);	/* For use in panic() or kernel exceptions */

#endif /* __DAVIX_PRINTK_H */
