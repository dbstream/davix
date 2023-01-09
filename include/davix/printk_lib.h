/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_PRINTK_LIB_H
#define __DAVIX_PRINTK_LIB_H

#include <davix/types.h>

/*
 * Callback function for printk().
 */
struct printk_callback {
	/*
	 * Called for every output character from raw_printk(). If this function
	 * returns non-zero, raw_printk returns instantly with a non-zero return
	 * value.
	 */
	int (*putc)(struct printk_callback *cb, char c);
};

int raw_vprintk(struct printk_callback *cb, const char *fmt, va_list *ap);
int raw_printk(struct printk_callback *cb, const char *fmt, ...);

void vsnprintk(char *buf, unsigned long bufsiz, const char *fmt, va_list ap);
void snprintk(char *buf, unsigned long bufsiz, const char *fmt, ...);

#endif /* __DAVIX_PRINTK_LIB_H */
