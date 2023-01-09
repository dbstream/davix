/* SPDX-License-Identifier: MIT */
#ifndef __ASM_BACKTRACE_H
#define __ASM_BACKTRACE_H

#include <asm/page.h>

struct stack_frame {
	struct stack_frame *next;
	unsigned long ret_ip;
};

#define backtrace_current(void) ({ \
	struct stack_frame *ret; \
	asm volatile("movq %%rbp, %0" : "=r"(ret)); \
	ret; \
})

static inline struct stack_frame *backtrace_next(struct stack_frame *frame)
{
	return frame->next;
}

static inline unsigned long backtrace_ip(struct stack_frame *frame)
{
	return frame->ret_ip;
}

/*
 * Check the validity of a stack frame.
 * Return value: non-zero on good stack frame.
 *
 * On x86, kernel stacks will always be either in the HHDM or in the kernel and
 * module mapping space.
 */
static inline int backtrace_check(struct stack_frame *frame)
{
	unsigned long addr = (unsigned long) frame;
	if(addr < HHDM_OFFSET)
		return 0;

	if(addr > 0xffffffff80000000 && addr < 0xffffffffffdfffff)
		return 1;

	if(addr > (l5_paging_enable
		? 0xff7fffffffffffffUL
		: 0xffffbfffffffffffUL))
			return 0;

	return 1;
}

#endif /* __ASM_BACKTRACE_H */
