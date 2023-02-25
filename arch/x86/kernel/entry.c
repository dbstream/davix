/* SPDX-License-Identifier: MIT */
#include <davix/setup.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <asm/entry.h>
#include <asm/segment.h>

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
	debug("entry: Interrupt vector %u called.\n", frame->excep_nr);
}

void x86_spurious(struct irq_frame *frame);
void x86_spurious(struct irq_frame *frame)
{
	debug("entry: Spurious interrupt called.\n");
}

extern unsigned long x86_isr_stubs[];

static struct idt_entry idt[256];

static void set_idt_gate(unsigned int idx, unsigned long addr,
	unsigned long ist, bool trap)
{
	idt[idx] = (struct idt_entry) {
		.offset1 = addr,
		.cs = GDT_KERNEL_CS,
		.attribs = 0b10001110 | trap,
		.ist_entry = 0,
		.reserved = 0,
		.offset2 = addr >> 16,
		.offset3 = addr >> 32
	};
}

void arch_setup_interrupts(void)
{
#define IDT(i, ist, trap) set_idt_gate(i, x86_isr_stubs[i], ist, trap)

	/*
	 * Exceptions...
	 */
	for(unsigned int i = 0; i < 32; i++)
		IDT(i, 0, 1);

	/*
	 * ... except for NMI, MCE and doublefault.
	 */
	IDT(2, NMI_IST, 0);
	IDT(8, DOUBLEFAULT_IST, 0);
	IDT(18, NMI_IST, 0);

	/*
	 * Hardware IRQ vectors and similar.
	 */
	for(unsigned int i = 32; i < 256; i++)
		IDT(i, IRQ_IST, 0);

#undef IDT

	load_idt((unsigned long) &idt[0], sizeof(idt) - 1);
}
