/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_PANIC_H
#define __DAVIX_PANIC_H

#include <davix/types.h>
#include <asm/backtrace.h>

void vpanic_frame(struct stack_frame *frame, const char *fmt, va_list *ap);
void panic_frame(struct stack_frame *frame, const char *fmt, ...);
void panic(const char *fmt, ...);

#endif
