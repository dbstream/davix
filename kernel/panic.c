/* SPDX-License-Identifier: MIT */
#include <davix/atomic.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <asm/irq.h>

static atomic unsigned long in_panic = 0;

static inline void enter_panic(void)
{
	unsigned long e = 0;
	if(atomic_compare_exchange_strong(&in_panic, &e, 1,
		memory_order_seq_cst, memory_order_seq_cst)) {
			/*
			 * We are the panicking CPU, just return.
			 */
			return;
	}

	/*
	 * Another CPU panicked before us, just go to sleep.
	 */
	for(;;)
		relax();
}

void vpanic_frame(struct stack_frame *frame, const char *fmt, va_list *ap)
{
	disable_interrupts();

	enter_panic();
	printk_bust_locks();

	critical("\n");
	critical("***********************\n");
	critical("*** KERNEL PANIC!!! ***\n");
	critical("***********************\n");
	critical("\n");
	critical("%#pV\n", fmt, ap);
	critical("\n");
	info("---[ Backtrace start ]---\n");
	for(unsigned count = 0;; count++) {
		if(!frame) {
			if(count == 0)
				info("        (empty backtrace)\n");
			break;
		}

		if(!backtrace_check(frame)) {
			info("        Bad stack frame: %p\n", frame);
			break;
		}

		if(count >= 20) {
			info("        (... more)\n");
			break;
		}

		info("        [%2u] %p\n", count, backtrace_ip(frame));
		frame = backtrace_next(frame);
	}
	info("---[ Backtrace end ]---\n");
	info("\n");
	for(;;)
		relax();
}

void panic_frame(struct stack_frame *frame, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vpanic_frame(frame, fmt, &ap);
	va_end(ap);
}

void panic(const char *fmt, ...)
{
	struct stack_frame *frame = backtrace_current();
	va_list ap;
	va_start(ap, fmt);
	vpanic_frame(frame, fmt, &ap);
	va_end(ap);
}
