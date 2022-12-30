/* SPDX-License-Identifier: MIT */
#include <davix/printk.h>
#include <asm/entry.h>
#include <asm/segment.h>

extern unsigned long early_isr_stubs[];

static struct idt_entry early_idt[32];

static void dump_registers(struct irq_frame *frame)
{
	info("Registers:\n");
	info("     rax=%p     rbx=%p     rcx=%p     rdx=%p\n",
		frame->rax, frame->rbx, frame->rcx, frame->rdx);
	info("     rdi=%p     rsi=%p     rbp=%p     rsp=%p\n",
		frame->rdi, frame->rsi, frame->rbp, frame->rsp);
	info("      r8=%p      r9=%p     r10=%p     r11=%p\n",
		frame->r8, frame->r9, frame->r10, frame->r11);
	info("     r12=%p     r13=%p     r14=%p     r15=%p\n",
		frame->r12, frame->r13, frame->r14, frame->r15);
	info("  rflags=%p      ip=%p\n", frame->rflags, frame->rip);
}

void __handle_early_nmi(struct irq_frame *frame);
void __handle_early_nmi(struct irq_frame *frame)
{
	printk_bust_locks();

	critical("\n");
	if(frame->excep_nr == X86_TRAP_NMI) {
		critical("=====================================\n");
		critical(" caught non-maskable interrupt (NMI)\n");
		critical("=====================================\n");
	} else {
		critical("==================================\n");
		critical(" caught machine-check error (MCE)\n");
		critical(" error code 0x%x\n", frame->error);
		critical("==================================\n");
	}

	dump_registers(frame);

	for(;;)
		relax();
}

void __handle_early_exception(struct irq_frame *frame);
void __handle_early_exception(struct irq_frame *frame)
{
	printk_bust_locks();

	critical("\n");
	critical("=======================\n");
	critical(" caught exception 0x%x\n", frame->excep_nr);
	critical(" error code 0x%x\n", frame->error);
	critical("=======================\n");

	dump_registers(frame);

	for(;;)
		relax();
}

void __handle_early_pagefault(struct irq_frame *frame);
void __handle_early_pagefault(struct irq_frame *frame)
{
	printk_bust_locks();

	int present = frame->error & 1;
	int write = frame->error & 2;
	int ifetch = frame->error & 16;

	critical("\n");
	critical("=========================================================\n");
	critical(" PAGE FAULT!\n");
	critical(" Caused by %s %p\n",
		present
			? ifetch
				? "instruction fetch from no-execute address"
				: "write to read-only address"
			: ifetch
				? "instruction fetch from non-present address"
				: write
					? "write to non-present address"
					: "read from non-present-address",
		read_cr2());
	critical("=========================================================\n");

	dump_registers(frame);

	for(;;)
		relax();
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
