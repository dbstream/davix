/* SPDX-License-Identifier: MIT */
#include <davix/atomic.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <asm/irq.h>

static atomic unsigned panic_cpu = -1;

static inline bool enter_panic(void)
{
	unsigned e = -1;
	unsigned no = smp_self()->id;
	if(atomic_compare_exchange_strong(&panic_cpu, &e, no,
		memory_order_seq_cst, memory_order_seq_cst)) {
			/*
			 * We are the panicking CPU, just return.
			 */
			return 1;
	}

	/*
	 * Another CPU panicked before us.
	 */
	return 0;
}

static void panic_stop_self(void)
{
	atomic_store(&smp_self()->online, 0, memory_order_seq_cst);
	for(;;)
		relax();
}

static void panic_stop_others(void)
{
	struct logical_cpu *self = smp_self();
	for_each_logical_cpu(cpu) {
		if(cpu == self)
			continue;
		if(!cpu->online)
			continue;

		smp_send_nmi(cpu);
		do {
			relax();
		} while(atomic_load(&cpu->online, memory_order_acquire));
	}
}

static void do_panic_prints(struct stack_frame *frame,
	const char *fmt, va_list *ap)
{
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
}

void vpanic_frame(struct stack_frame *frame, const char *fmt, va_list *ap)
{
	disable_interrupts();
	preempt_disable(); /* stay safe */

	if(!enter_panic())
		panic_stop_self();
	panic_stop_others();
	printk_bust_locks();
	do_panic_prints(frame, fmt, ap);
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

void panic_nmi(struct stack_frame *frame, const char *fmt, ...)
{
	unsigned e = -1;
	unsigned no = smp_self()->id;
	if(!atomic_compare_exchange_strong(&panic_cpu, &e, no,
		memory_order_seq_cst, memory_order_seq_cst)) {
		/*
		 * We got NMI'd while panicking. Just return.
		 */
		if(e == no)
			return;

		panic_stop_self();
	}

	preempt_disable();
	panic_stop_others();
	printk_bust_locks();

	va_list ap;
	va_start(ap, fmt);
	do_panic_prints(frame, fmt, &ap);
	va_end(ap);

	for(;;)
		relax();
}
