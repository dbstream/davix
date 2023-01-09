/* SPDX-License-Identifier: MIT */
#include <davix/panic.h>
#include <davix/printk.h>
#include <asm/entry.h>
#include <asm/segment.h>

extern unsigned long early_isr_stubs[];

static struct idt_entry early_idt[32];

static struct stack_frame create_fake_frame(unsigned long rbp,
	unsigned long rip)
{
	return (struct stack_frame) {(struct stack_frame *) rbp, rip};
}

void __handle_early_nmi(struct irq_frame *frame);
void __handle_early_nmi(struct irq_frame *frame)
{
	struct stack_frame btframe = create_fake_frame(frame->rbp, frame->rip);
	if(frame->excep_nr == X86_TRAP_NMI) {
		panic_frame(&btframe, "Caught NMI");
	} else {
		panic_frame(&btframe, "Caught MCE %#x",
			frame->error);
	}
}

void __handle_early_exception(struct irq_frame *frame);
void __handle_early_exception(struct irq_frame *frame)
{
	struct stack_frame btframe = create_fake_frame(frame->rbp, frame->rip);
	panic_frame(&btframe, "Caught exception %x (error code %x)\n",
		frame->excep_nr, frame->error);
}

void __handle_early_pagefault(struct irq_frame *frame);
void __handle_early_pagefault(struct irq_frame *frame)
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

	struct stack_frame btframe = create_fake_frame(frame->rbp, frame->rip);
	panic_frame(&btframe, "Page fault caused by %s %p\n",
		cause, read_cr2());
}

static void set_idt_gate(unsigned idx, unsigned long addr)
{
	early_idt[idx] = (struct idt_entry) {
		.offset1 = addr,
		.cs = GDT_KERNEL_CS,
		.attribs = 0b10001110,
		.ist_entry = 0,
		.reserved = 0,
		.offset2 = addr >> 16,
		.offset3 = addr >> 32
	};
}

void x86_setup_early_idt(void)
{
	for(unsigned i = 0; i < 32; i++)
		set_idt_gate(i, early_isr_stubs[i]);

	info("x86/kernel: Loading early interrupt descriptor table...\n");
	load_idt((unsigned long) &early_idt[0], sizeof(early_idt) - 1);
}
