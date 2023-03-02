/* SPDX-License-Identifier: MIT */
#include <davix/setup.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/time.h>
#include <davix/sched.h>
#include <asm/apic.h>
#include <asm/entry.h>
#include <asm/segment.h>

/*
 * NOTE on preempt_{disable/enable} usage:
 *
 * preempt_enable() will call maybe_resched(). This means that, if there is a
 * preempt_{disable/enable} pair within any code path in an interrupt handler
 * and preemption wasn't disabled when the interrupt was delivered, we might
 * potentially reschedule right in the middle of an interrupt handler, which
 * would be bad.
 */

static inline struct stack_frame create_fake_frame(unsigned long rbp,
	unsigned long rip)
{
	return (struct stack_frame) {(struct stack_frame *) rbp, rip};
}

void x86_ex_unhandled(struct irq_frame *frame);
void x86_ex_unhandled(struct irq_frame *frame)
{
	struct stack_frame bt_frame = create_fake_frame(frame->rbp, frame->rip);
	panic_frame(&bt_frame, "Caught exception 0x%x", frame->excep_nr);
}

void x86_gp_fault(struct irq_frame *frame);
void x86_gp_fault(struct irq_frame *frame)
{
	struct stack_frame bt_frame = create_fake_frame(frame->rbp, frame->rip);
	panic_frame(&bt_frame, "Caught general-protection fault (error code %x)",
		frame->error);
}

void x86_page_fault(struct irq_frame *frame);
void x86_page_fault(struct irq_frame *frame)
{
	int present = frame->error & 1;
	int write = frame->error & 2;
	int ifetch = frame->error & 16;

	const char *cause = present
		? ifetch
			? "instruction fetch from no-execute address"
			: "write to read-only address"
		: ifetch
			? "instruction fetch from non-present address"
			: write
				? "write to non-present address"
				: "read from non-present-address";

	struct stack_frame bt_frame = create_fake_frame(frame->rbp, frame->rip);
	panic_frame(&bt_frame, "Caught page fault (%s %p)",
		cause, read_cr2());
}

void x86_doublefault(struct irq_frame *frame);
void x86_doublefault(struct irq_frame *frame)
{
	panic("Caught double-fault. If the kernel is miraculously alive to print this, I'll be surprised.");
}

void x86_nmi(struct irq_frame *frame);
void x86_nmi(struct irq_frame *frame)
{
	struct stack_frame bt_frame = create_fake_frame(frame->rbp, frame->rip);
	panic_frame(&bt_frame, "Caught non-maskable interrupt");
}

void x86_mce(struct irq_frame *frame);
void x86_mce(struct irq_frame *frame)
{
	struct stack_frame bt_frame = create_fake_frame(frame->rbp, frame->rip);
	panic_frame(&bt_frame, "Caught machine-check error 0x%x",
		frame->error);
}

void x86_irq(struct irq_frame *frame);
void x86_irq(struct irq_frame *frame)
{
	preempt_disable();
	debug("entry: Interrupt vector %u called.\n", frame->excep_nr);

	apic_send_eoi();
	preempt_enable(); /* This will call maybe_resched() for us. */
}

void x86_timer(struct irq_frame *frame);
void x86_timer(struct irq_frame *frame)
{
	preempt_disable();

	apic_send_eoi();
	timer_interrupt();

	preempt_enable();
}

void x86_spurious(struct irq_frame *frame);
void x86_spurious(struct irq_frame *frame)
{
	preempt_disable();
	debug("entry: Spurious interrupt called.\n");
	preempt_enable();
}

extern unsigned long x86_isr_stubs[];

static struct idt_entry idt[256];

static void set_idt_gate(unsigned int idx, unsigned long addr,
	unsigned long ist)
{
	idt[idx] = (struct idt_entry) {
		.offset1 = addr,
		.cs = GDT_KERNEL_CS,
		.attribs = 0b10001110,
		.ist_entry = 0,
		.reserved = 0,
		.offset2 = addr >> 16,
		.offset3 = addr >> 32
	};
}

void arch_setup_interrupts(void)
{
#define IDT(i, ist) set_idt_gate(i, x86_isr_stubs[i], ist)

	/*
	 * Exceptions...
	 */
	for(unsigned int i = 0; i < 32; i++)
		IDT(i, 0);

	/*
	 * ... except for NMI, MCE and doublefault.
	 */
	IDT(2, NMI_IST);
	IDT(8, DOUBLEFAULT_IST);
	IDT(18, NMI_IST);

	/*
	 * Hardware IRQ vectors and similar.
	 */
	for(unsigned int i = 32; i < 256; i++)
		IDT(i, IRQ_IST);

#undef IDT

	load_idt((unsigned long) &idt[0], sizeof(idt) - 1);

	/* now we are ready to handle the timer interrupt */
	apic_configure();
}
