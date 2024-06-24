/**
 * Kernel printk().
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_PRINTK_H
#define _DAVIX_PRINTK_H 1

#define PR_ASCII_SOH "\x01"	/* ASCII Start-Of-Header */
#define PR_ASCII_STX "\x02"	/* ASCII Start-Of-Text */

#define PR_NONE		PR_ASCII_SOH "0" PR_ASCII_STX
#define PR_INFO		PR_ASCII_SOH "1" PR_ASCII_STX
#define PR_NOTICE	PR_ASCII_SOH "2" PR_ASCII_STX
#define PR_WARN		PR_ASCII_SOH "3" PR_ASCII_STX
#define PR_ERR		PR_ASCII_SOH "4" PR_ASCII_STX
#define PR_CRIT		PR_ASCII_SOH "5" PR_ASCII_STX

__attribute__ ((format (printf, 1, 2)))
extern void
printk (const char *fmt, ...);

#endif /* _DAVIX_PRINTK_H */

/**
 * Define vprintk() if the user has included stdarg. Keep this in its own
 * guard macro for users who do the following:
 *	#include <davix/printk.h>
 *	#include <davix/stdarg.h>
 *	#include <davix/printk.h>
 * The above scenario can be tricky when inclusions of printk are hidden in
 * other header files, so make sure it works.
 */
#ifdef _DAVIX_STDARG_H
#ifndef _VPRINTK_DEFINED
#define _VPRINTK_DEFINED 1

__attribute__ ((format (printf, 1, 0)))
extern void
vprintk (const char *fmt, va_list args);

#endif
#endif
